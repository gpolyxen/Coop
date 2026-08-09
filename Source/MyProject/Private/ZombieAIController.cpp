#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "ShooterCharacter.h"
#include "HealthArmorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	ResetNavigationRecovery();
}

void AZombieAIController::ResetNavigationRecovery()
{
	bUsingDetour=false;
	DetourLocation=FVector::ZeroVector;
	DetourExpireTime=0.;
	NextPathRefreshTime=0.;
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
	ResetNavigationRecovery();
}

void AZombieAIController::OnTargetPerception(AActor* Actor,FAIStimulus Stimulus)
{
	if(!Cast<AShooterCharacter>(Actor)||Actor==GetPawn())return;
	if(Stimulus.WasSuccessfullySensed())
	{
		const bool bNewTarget=Target.Get()!=Actor;
		Target=Actor;
		LastKnownLocation=Stimulus.StimulusLocation.IsNearlyZero()?Actor->GetActorLocation():Stimulus.StimulusLocation;
		LastStimulus=GetWorld()->GetTimeSeconds();
		if(bNewTarget)
		{
			bPatrolMoveActive=false;
			bSteeringPatrol=false;
			LastChaseProgressLocation=GetPawn()?GetPawn()->GetActorLocation():FVector::ZeroVector;
			LastChaseProgressTime=GetWorld()->GetTimeSeconds();
			ResetNavigationRecovery();
			UE_LOG(LogTemp,Display,TEXT("Zombie perception acquired %s"),*GetNameSafe(Actor));
		}
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

bool AZombieAIController::TryJumpObstacle(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie||!GetWorld())return false;
	UCharacterMovementComponent* Movement=Zombie->GetCharacterMovement();
	UCapsuleComponent* Capsule=Zombie->GetCapsuleComponent();
	const double Now=GetWorld()->GetTimeSeconds();
	if(!Movement||!Capsule||Movement->IsFalling()||!Movement->IsMovingOnGround()||Now-LastJumpTime<JumpCooldown)return false;

	FVector TravelDirection=Zombie->GetVelocity();
	TravelDirection.Z=0.f;
	if(TravelDirection.SizeSquared2D()<FMath::Square(35.f))
	{
		TravelDirection=Destination-Zombie->GetActorLocation();
		TravelDirection.Z=0.f;
	}
	if(!TravelDirection.Normalize())return false;

	float CapsuleRadius=0.f;
	float CapsuleHalfHeight=0.f;
	Capsule->GetScaledCapsuleSize(CapsuleRadius,CapsuleHalfHeight);
	const FVector ActorLocation=Zombie->GetActorLocation();
	const float GroundZ=ActorLocation.Z-CapsuleHalfHeight;
	const float ProbeRadius=FMath::Clamp(CapsuleRadius*.55f,18.f,28.f);
	const FVector ProbeStart(ActorLocation.X,ActorLocation.Y,GroundZ+Movement->MaxStepHeight+ProbeRadius*.75f);
	const FVector ProbeEnd=ProbeStart+TravelDirection*JumpProbeDistance;

	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ZombieJumpProbe),false,Zombie);
	if(Target.IsValid())Query.AddIgnoredActor(Target.Get());

	FHitResult ObstacleHit;
	if(!GetWorld()->SweepSingleByObjectType(ObstacleHit,ProbeStart,ProbeEnd,FQuat::Identity,ObjectTypes,FCollisionShape::MakeSphere(ProbeRadius),Query))return false;
	if(ObstacleHit.bStartPenetrating||ObstacleHit.ImpactNormal.Z>=Movement->GetWalkableFloorZ())return false;

	const FVector TopProbePoint=ObstacleHit.ImpactPoint+TravelDirection*(ProbeRadius+8.f);
	const FVector TopTraceStart(TopProbePoint.X,TopProbePoint.Y,GroundZ+MaximumJumpObstacleHeight+60.f);
	const FVector TopTraceEnd(TopProbePoint.X,TopProbePoint.Y,GroundZ+1.f);
	FHitResult TopHit;
	if(!GetWorld()->LineTraceSingleByObjectType(TopHit,TopTraceStart,TopTraceEnd,ObjectTypes,Query))return false;
	const float ObstacleHeight=TopHit.ImpactPoint.Z-GroundZ;
	if(ObstacleHeight<=FMath::Max(MinimumJumpObstacleHeight,Movement->MaxStepHeight+2.f)||ObstacleHeight>MaximumJumpObstacleHeight)return false;
	if(TopHit.ImpactNormal.Z<Movement->GetWalkableFloorZ())return false;

	const float Gravity=FMath::Abs(Movement->GetGravityZ());
	if(Gravity<=KINDA_SMALL_NUMBER)return false;
	const float MaximumBallisticRise=FMath::Square(Movement->JumpZVelocity)/(2.f*Gravity);
	const float RequiredRise=ObstacleHeight+JumpClearance;
	if(RequiredRise>MaximumBallisticRise*.9f)return false;

	const FCollisionShape ClearanceCapsule=FCollisionShape::MakeCapsule(FMath::Max(8.f,CapsuleRadius-4.f),FMath::Max(12.f,CapsuleHalfHeight-6.f));
	FHitResult ClearanceHit;
	const FVector RaisedLocation=ActorLocation+FVector(0.f,0.f,RequiredRise);
	if(GetWorld()->SweepSingleByObjectType(ClearanceHit,ActorLocation,RaisedLocation,FQuat::Identity,ObjectTypes,ClearanceCapsule,Query))return false;

	const float LandingDistance=JumpProbeDistance*2.f;
	const FVector RaisedEnd=RaisedLocation+TravelDirection*LandingDistance;
	if(GetWorld()->SweepSingleByObjectType(ClearanceHit,RaisedLocation,RaisedEnd,FQuat::Identity,ObjectTypes,ClearanceCapsule,Query))return false;

	const FVector LandingProbe=ActorLocation+TravelDirection*LandingDistance;
	const FVector LandingTraceStart(LandingProbe.X,LandingProbe.Y,GroundZ+MaximumBallisticRise+60.f);
	const FVector LandingTraceEnd(LandingProbe.X,LandingProbe.Y,GroundZ-MaximumLandingDrop);
	FHitResult LandingHit;
	if(!GetWorld()->LineTraceSingleByObjectType(LandingHit,LandingTraceStart,LandingTraceEnd,ObjectTypes,Query))return false;
	const float LandingHeight=LandingHit.ImpactPoint.Z-GroundZ;
	if(LandingHeight>MaximumJumpObstacleHeight||LandingHeight<-MaximumLandingDrop||LandingHit.ImpactNormal.Z<Movement->GetWalkableFloorZ())return false;

	StopMovement();
	Movement->StopMovementImmediately();
	JumpTravelDirection=TravelDirection;
	LastJumpTime=Now;
	bUsingDetour=false;
	NextPathRefreshTime=Now+PathRefreshInterval;
	Zombie->LaunchCharacter(TravelDirection*JumpForwardVelocity+FVector(0.f,0.f,Movement->JumpZVelocity),true,true);
	UE_LOG(LogTemp,Display,TEXT("Zombie %s jumps over %.0f cm obstacle"),*GetNameSafe(Zombie),ObstacleHeight);
	return true;
}

bool AZombieAIController::TryStartDetour(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld());
	if(!Zombie||!Navigation)return false;

	FVector Forward=Destination-Zombie->GetActorLocation();
	Forward.Z=0.f;
	if(!Forward.Normalize())return false;
	const FVector Right=FVector::CrossProduct(FVector::UpVector,Forward).GetSafeNormal();
	const FVector Start=Zombie->GetActorLocation();
	const float Side=DetourSearchRadius;
	const float Ahead=DetourSearchRadius*.55f;
	TArray<FVector> Candidates;
	Candidates.Reserve(6);
	Candidates.Add(Start+Forward*Ahead+Right*Side);
	Candidates.Add(Start+Forward*Ahead-Right*Side);
	Candidates.Add(Start+Right*Side);
	Candidates.Add(Start-Right*Side);
	Candidates.Add(Start+Forward*DetourSearchRadius+Right*Side*.55f);
	Candidates.Add(Start+Forward*DetourSearchRadius-Right*Side*.55f);

	bool bFoundCandidate=false;
	float BestScore=MAX_flt;
	FVector BestLocation=FVector::ZeroVector;
	for(const FVector& Candidate:Candidates)
	{
		FNavLocation Projected;
		if(!Navigation->ProjectPointToNavigation(Candidate,Projected,FVector(180.f,180.f,300.f)))continue;
		UNavigationPath* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),Start,Projected.Location,Zombie);
		if(!Path||!Path->IsValid()||Path->IsPartial())continue;
		const float Score=FVector::DistSquared2D(Projected.Location,Destination)+FVector::DistSquared2D(Projected.Location,Start)*.2f;
		if(Score<BestScore)
		{
			BestScore=Score;
			BestLocation=Projected.Location;
			bFoundCandidate=true;
		}
	}
	if(!bFoundCandidate)return false;

	const EPathFollowingRequestResult::Type MoveResult=MoveToLocation(BestLocation,PatrolAcceptanceRadius,true,true,true);
	if(MoveResult==EPathFollowingRequestResult::Failed)return false;
	bUsingDetour=true;
	DetourLocation=BestLocation;
	DetourExpireTime=GetWorld()->GetTimeSeconds()+DetourTimeout;
	NextPathRefreshTime=DetourExpireTime;
	UE_LOG(LogTemp,Display,TEXT("Zombie %s uses a NavMesh detour"),*GetNameSafe(Zombie));
	return true;
}

void AZombieAIController::SteerToward(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie)return;
	if(TryJumpObstacle(Destination))return;
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
		ResetNavigationRecovery();
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
	UCharacterMovementComponent* Movement=Zombie->GetCharacterMovement();
	if(Movement&&Movement->IsFalling())
	{
		if(!JumpTravelDirection.IsNearlyZero())Zombie->AddMovementInput(JumpTravelDirection,1.f);
		return;
	}
	if(!JumpTravelDirection.IsNearlyZero())
	{
		JumpTravelDirection=FVector::ZeroVector;
		NextPathRefreshTime=0.;
	}

	if(Target.IsValid())
	{
		if(UHealthArmorComponent* TargetHealth=Target->FindComponentByClass<UHealthArmorComponent>())
			if(TargetHealth->IsDead())
			{
				Target.Reset();
				ResetNavigationRecovery();
			}
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
			ResetNavigationRecovery();
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
			else
			{
				const FVector Destination=bCanSeeTarget?Target->GetActorLocation():LastKnownLocation;
				if(bUsingDetour)
				{
					const bool bDetourFinished=FVector::DistSquared2D(Zombie->GetActorLocation(),DetourLocation)<=FMath::Square(PatrolAcceptanceRadius)
						||GetMoveStatus()==EPathFollowingStatus::Idle||Now>=DetourExpireTime;
					if(!bDetourFinished)
					{
						TryJumpObstacle(DetourLocation);
						return;
					}
					bUsingDetour=false;
					NextPathRefreshTime=0.;
				}

				if(TryJumpObstacle(Destination))return;
				if(Now-LastChaseProgressTime>=StuckRecoveryDelay)
				{
					if(TryStartDetour(Destination))return;
					LastChaseProgressLocation=Zombie->GetActorLocation();
					LastChaseProgressTime=Now;
					NextPathRefreshTime=0.;
				}

				if(GetMoveStatus()==EPathFollowingStatus::Idle||Now>=NextPathRefreshTime)
				{
					const EPathFollowingRequestResult::Type MoveResult=bCanSeeTarget
						?MoveToActor(Target.Get(),Zombie->AttackRange*.75f,true,true,true)
						:MoveToLocation(LastKnownLocation,PatrolAcceptanceRadius,true,true,true);
					NextPathRefreshTime=Now+PathRefreshInterval;
					if(MoveResult==EPathFollowingRequestResult::Failed&&!TryStartDetour(Destination))SteerToward(Destination);
				}
			}
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
