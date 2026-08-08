#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "ShooterCharacter.h"
#include "HealthArmorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "EngineUtils.h"

AZombieAIController::AZombieAIController()
{
	PrimaryActorTick.bCanEverTick=true;
	Senses=CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*Senses);
	Sight=CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));
	Sight->SightRadius=2200.f;
	Sight->LoseSightRadius=2800.f;
	Sight->PeripheralVisionAngleDegrees=75.f;
	Sight->SetMaxAge(ForgetAfter);
	Sight->DetectionByAffiliation.bDetectEnemies=true;
	Sight->DetectionByAffiliation.bDetectFriendlies=true;
	Sight->DetectionByAffiliation.bDetectNeutrals=true;
	Hearing=CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));
	Hearing->HearingRange=4000.f;
	Hearing->SetMaxAge(ForgetAfter);
	Hearing->DetectionByAffiliation.bDetectEnemies=true;
	Hearing->DetectionByAffiliation.bDetectFriendlies=true;
	Hearing->DetectionByAffiliation.bDetectNeutrals=true;
	Senses->ConfigureSense(*Sight);
	Senses->ConfigureSense(*Hearing);
	Senses->SetDominantSense(Sight->GetSenseImplementation());
	Senses->OnTargetPerceptionUpdated.AddDynamic(this,&AZombieAIController::OnTargetPerception);
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	NextPatrolTime=GetWorld()?GetWorld()->GetTimeSeconds()+FMath::FRandRange(PatrolWaitMin,PatrolWaitMax):0.;
}

void AZombieAIController::AlertToActor(AActor* Actor)
{
	AShooterCharacter* Player=Cast<AShooterCharacter>(Actor);
	if(!Player)return;
	Target=Player;
	LastKnownLocation=Player->GetActorLocation();
	LastStimulus=GetWorld()->GetTimeSeconds();
	bPatrolMoveActive=false;
	bSteeringPatrol=false;
	LastChaseProgressLocation=GetPawn()?GetPawn()->GetActorLocation():FVector::ZeroVector;
	LastChaseProgressTime=GetWorld()->GetTimeSeconds();
	bDirectChase=false;
}

void AZombieAIController::OnTargetPerception(AActor* Actor,FAIStimulus Stimulus)
{
	if(!Cast<AShooterCharacter>(Actor)||Actor==GetPawn())return;
	if(Stimulus.WasSuccessfullySensed())
	{
		Target=Actor;
		LastKnownLocation=Stimulus.StimulusLocation.IsNearlyZero()?Actor->GetActorLocation():Stimulus.StimulusLocation;
		LastStimulus=GetWorld()->GetTimeSeconds();
		bPatrolMoveActive=false;
		bSteeringPatrol=false;
		LastChaseProgressLocation=GetPawn()?GetPawn()->GetActorLocation():FVector::ZeroVector;
		LastChaseProgressTime=GetWorld()->GetTimeSeconds();
		bDirectChase=false;
		UE_LOG(LogTemp,Display,TEXT("Zombie perception sensed %s"),*GetNameSafe(Actor));
	}
}

void AZombieAIController::TryStartPatrol()
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld());
	if(!Zombie||!Navigation)return;
	FNavLocation PatrolPoint;
	if(Navigation->GetRandomReachablePointInRadius(Zombie->GetActorLocation(),PatrolRadius,PatrolPoint))
	{
		bPatrolMoveActive=MoveToLocation(PatrolPoint.Location,PatrolAcceptanceRadius,true,true,true)!=EPathFollowingRequestResult::Failed;
		if(!bPatrolMoveActive)NextPatrolTime=GetWorld()->GetTimeSeconds()+1.f;
	}
	else
	{
		const FVector2D Direction=FMath::RandPointInCircle(PatrolRadius);
		SteeringPatrolLocation=Zombie->GetActorLocation()+FVector(Direction.X,Direction.Y,0.f);
		bSteeringPatrol=true;
		bPatrolMoveActive=true;
	}
}

void AZombieAIController::SteerToward(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie)return;
	FVector DesiredDirection=Destination-Zombie->GetActorLocation();
	DesiredDirection.Z=0.f;
	if(!DesiredDirection.Normalize())return;

	const FVector TraceStart=Zombie->GetActorLocation()+FVector(0.f,0.f,50.f);
	const FVector TraceEnd=TraceStart+DesiredDirection*180.f;
	FHitResult Obstacle;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ZombieSteering),false,Zombie);
	if(GetWorld()->SweepSingleByChannel(Obstacle,TraceStart,TraceEnd,FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeSphere(42.f),Query))
	{
		FVector SlideDirection=FVector::VectorPlaneProject(DesiredDirection,Obstacle.ImpactNormal);
		SlideDirection.Z=0.f;
		if(SlideDirection.Normalize())DesiredDirection=SlideDirection;
		else DesiredDirection=DesiredDirection.RotateAngleAxis(55.f,FVector::UpVector);
	}
	Zombie->AddMovementInput(DesiredDirection,1.f);
}

void AZombieAIController::TryAcquireVisibleTarget()
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie||!Sight)return;
	const FVector EyesLocation=Zombie->GetActorLocation()+FVector(0.f,0.f,55.f);
	const float MinimumFacingDot=FMath::Cos(FMath::DegreesToRadians(Sight->PeripheralVisionAngleDegrees));
	AShooterCharacter* BestTarget=nullptr;
	float BestDistanceSquared=FMath::Square(Sight->SightRadius);
	int32 PlayerCount=0;
	for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)
	{
		AShooterCharacter* Player=*It;
		++PlayerCount;
		if(!Player||Player->IsDead())continue;
		FVector ToPlayer=Player->GetActorLocation()-EyesLocation;
		const float DistanceSquared=ToPlayer.SizeSquared();
		ToPlayer.Z=0.f;
		if(!bLoggedInitialSightScan)UE_LOG(LogTemp,Display,TEXT("Zombie sight scan: player=%s distance=%.0f facing=%.2f line-of-sight=%s"),*GetNameSafe(Player),FMath::Sqrt(DistanceSquared),FVector::DotProduct(Zombie->GetActorForwardVector(),ToPlayer.GetSafeNormal()),LineOfSightTo(Player,EyesLocation)?TEXT("true"):TEXT("false"));
		if(DistanceSquared>BestDistanceSquared||ToPlayer.IsNearlyZero())continue;
		if(FVector::DotProduct(Zombie->GetActorForwardVector(),ToPlayer.GetSafeNormal())<MinimumFacingDot)continue;
		if(!LineOfSightTo(Player,EyesLocation))continue;
		BestTarget=Player;
		BestDistanceSquared=DistanceSquared;
	}
	if(!bLoggedInitialSightScan)
	{
		UE_LOG(LogTemp,Display,TEXT("Zombie initial sight scan found %d shooter character(s)"),PlayerCount);
		bLoggedInitialSightScan=true;
	}
	if(BestTarget)
	{
		Target=BestTarget;
		LastKnownLocation=BestTarget->GetActorLocation();
		LastStimulus=GetWorld()->GetTimeSeconds();
		bPatrolMoveActive=false;
		bSteeringPatrol=false;
		LastChaseProgressLocation=Zombie->GetActorLocation();
		LastChaseProgressTime=GetWorld()->GetTimeSeconds();
		bDirectChase=false;
		UE_LOG(LogTemp,Display,TEXT("Zombie %s acquired %s by sight"),*GetNameSafe(Zombie),*GetNameSafe(BestTarget));
	}
}

void AZombieAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie||Zombie->IsDead()){StopMovement();return;}
	if(Zombie->IsAttacking()){StopMovement();return;}
	const double Now=GetWorld()->GetTimeSeconds();

	if(Target.IsValid())
	{
		if(UHealthArmorComponent* TargetHealth=Target->FindComponentByClass<UHealthArmorComponent>())
			if(TargetHealth->IsDead())Target.Reset();
	}
	if(!Target.IsValid())TryAcquireVisibleTarget();

	if(Target.IsValid())
	{
		const bool bCanSeeTarget=LineOfSightTo(Target.Get());
		if(bCanSeeTarget)
		{
			LastKnownLocation=Target->GetActorLocation();
			LastStimulus=Now;
		}
		if(Now-LastStimulus>ForgetAfter)
		{
			Target.Reset();
			bDirectChase=false;
			StopMovement();
			NextPatrolTime=Now+FMath::FRandRange(PatrolWaitMin,PatrolWaitMax);
		}
		else
		{
			const float Distance=FVector::Dist(Zombie->GetActorLocation(),Target->GetActorLocation());
			if(FVector::DistSquared2D(Zombie->GetActorLocation(),LastChaseProgressLocation)>FMath::Square(25.f))
			{
				LastChaseProgressLocation=Zombie->GetActorLocation();
				LastChaseProgressTime=Now;
			}
			if(bCanSeeTarget&&Distance<=Zombie->AttackRange)
			{
				StopMovement();
				Zombie->TryAttack(Target.Get());
			}
			else if(bCanSeeTarget)
			{
				if(!bDirectChase)
				{
					const EPathFollowingRequestResult::Type MoveResult=MoveToActor(Target.Get(),Zombie->AttackRange*.75f,true,true,true);
					if(MoveResult==EPathFollowingRequestResult::Failed||Now-LastChaseProgressTime>.75)
					{
						UE_LOG(LogTemp,Display,TEXT("Zombie %s continuing chase with local obstacle steering"),*GetNameSafe(Zombie));
						bDirectChase=true;
						StopMovement();
					}
				}
				if(bDirectChase)SteerToward(Target->GetActorLocation());
			}
			else if(MoveToLocation(LastKnownLocation,PatrolAcceptanceRadius,true,true,true)==EPathFollowingRequestResult::Failed)SteerToward(LastKnownLocation);
			return;
		}
	}

	if(bSteeringPatrol)
	{
		if(FVector::DistSquared2D(Zombie->GetActorLocation(),SteeringPatrolLocation)<=FMath::Square(PatrolAcceptanceRadius))
		{
			bSteeringPatrol=false;
			bPatrolMoveActive=false;
			NextPatrolTime=Now+FMath::FRandRange(PatrolWaitMin,PatrolWaitMax);
		}
		else SteerToward(SteeringPatrolLocation);
		return;
	}

	if(bPatrolMoveActive&&GetMoveStatus()==EPathFollowingStatus::Idle)
	{
		bPatrolMoveActive=false;
		NextPatrolTime=Now+FMath::FRandRange(PatrolWaitMin,PatrolWaitMax);
	}
	if(!bPatrolMoveActive&&Now>=NextPatrolTime)TryStartPatrol();
}
