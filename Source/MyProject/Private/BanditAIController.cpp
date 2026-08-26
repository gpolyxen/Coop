#include "BanditAIController.h"
#include "BanditCharacter.h"
#include "BuildableStructure.h"
#include "ShooterCharacter.h"
#include "ZombieCharacter.h"
#include "HealthArmorComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EngineUtils.h"

ABanditAIController::ABanditAIController(){PrimaryActorTick.bCanEverTick=true;SetActorTickInterval(.12f);}

AActor* ABanditAIController::FindTarget()const
{
	AActor* Best=nullptr;float BestDistance=MAX_flt;const APawn* ControlledPawn=GetPawn();if(!ControlledPawn)return nullptr;
	for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)if(!It->IsDead())
	{
		const float Distance=FVector::DistSquared(ControlledPawn->GetActorLocation(),It->GetActorLocation());if(Distance<BestDistance){BestDistance=Distance;Best=*It;}
	}
	for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It)if(!It->IsDead())
	{
		const float Distance=FVector::DistSquared(ControlledPawn->GetActorLocation(),It->GetActorLocation());if(Distance<BestDistance){BestDistance=Distance;Best=*It;}
	}
	return Best;
}

bool ABanditAIController::FindCover(AActor* Target,FVector& OutCover)const
{
	const APawn* ControlledPawn=GetPawn();UNavigationSystemV1* Nav=UNavigationSystemV1::GetCurrent(GetWorld());if(!ControlledPawn||!Target||!Nav)return false;
	float BestScore=MAX_flt;bool bFound=false;
	for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)
	{
		ABuildableStructure* Structure=*It;if(!Structure||Structure->IsCollapsing()||Structure->IsConstructionPreview())continue;
		if(FVector::DistSquared2D(ControlledPawn->GetActorLocation(),Structure->GetActorLocation())>FMath::Square(CoverSearchRadius))continue;
		FVector Away=(Structure->GetActorLocation()-Target->GetActorLocation()).GetSafeNormal2D();if(Away.IsNearlyZero())continue;
		FNavLocation Projected;if(!Nav->ProjectPointToNavigation(Structure->GetActorLocation()+Away*240.f,Projected,FVector(160,160,220)))continue;
		UNavigationPath* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),ControlledPawn->GetActorLocation(),Projected.Location,const_cast<APawn*>(ControlledPawn));
		if(!Path||!Path->IsValid()||Path->IsPartial())continue;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(BanditCover),false,ControlledPawn);Query.AddIgnoredActor(Target);
		FHitResult Hit;const bool bBlocked=GetWorld()->LineTraceSingleByChannel(Hit,Projected.Location+FVector(0,0,55),Target->GetActorLocation()+FVector(0,0,55),ECC_Visibility,Query);
		if(!bBlocked)continue;const float Score=FVector::DistSquared2D(ControlledPawn->GetActorLocation(),Projected.Location);
		if(Score<BestScore){BestScore=Score;OutCover=Projected.Location;bFound=true;}
	}
	return bFound;
}

void ABanditAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);ABanditCharacter* Bandit=Cast<ABanditCharacter>(GetPawn());if(!Bandit||Bandit->IsDead()||!GetWorld())return;
	const double Now=GetWorld()->GetTimeSeconds();
	if(AZombieCharacter* ZombieTarget=Cast<AZombieCharacter>(TargetActor.Get()))if(ZombieTarget->IsDead())TargetActor.Reset();
	if(AShooterCharacter* PlayerTarget=Cast<AShooterCharacter>(TargetActor.Get()))if(PlayerTarget->IsDead())TargetActor.Reset();
	if(!TargetActor.IsValid()||Now>=NextTargetScan){TargetActor=FindTarget();NextTargetScan=Now+1.;}
	AActor* Target=TargetActor.Get();if(!Target)return;const float Distance=FVector::Dist(Bandit->GetActorLocation(),Target->GetActorLocation());
	if(Distance<=MeleeRange){StopMovement();Bandit->UnCrouch();Bandit->TryMelee(Target);return;}
	if(Distance>EngagementRange){Bandit->UnCrouch();MoveToActor(Target,EngagementRange*.7f,true,true,true);return;}
	if(CoverLocation.IsNearlyZero()||Now>=CoverExpires)
	{
		if(!FindCover(Target,CoverLocation))CoverLocation=Bandit->GetActorLocation();CoverExpires=Now+FMath::FRandRange(5.f,9.f);
	}
	if(FVector::DistSquared2D(Bandit->GetActorLocation(),CoverLocation)>FMath::Square(130.f)){Bandit->UnCrouch();MoveToLocation(CoverLocation,85.f,true,true,true);return;}
	StopMovement();if(Now>=NextShotTime)
	{
		Bandit->UnCrouch();Bandit->SetActorRotation((Target->GetActorLocation()-Bandit->GetActorLocation()).Rotation());
		if(LineOfSightTo(Target))Bandit->FireAt(Target);NextShotTime=Now+FMath::FRandRange(.65f,1.25f);RecrouchTime=Now+.28f;
	}
	else if(Now>=RecrouchTime)Bandit->Crouch();
}
