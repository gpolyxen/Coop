#include "ZombieSpawnManager.h"

#include "AmmoPickup.h"
#include "BruteZombieCharacter.h"
#include "HealthPickup.h"
#include "RunnerZombieCharacter.h"
#include "SpitterZombieCharacter.h"
#include "ShooterCharacter.h"
#include "ZombieCharacter.h"
#include "NavigationInvokerComponent.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AZombieSpawnManager::AZombieSpawnManager()
{
	PrimaryActorTick.bCanEverTick=false;
	bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(false);
}

void AZombieSpawnManager::BeginPlay()
{
	Super::BeginPlay();
	if(!HasAuthority())return;
	PreparationEndServerTime=GetWorld()->GetTimeSeconds()+PreparationDuration;
	for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It)It->Destroy();
	GetWorldTimerManager().SetTimer(ZombieTimer,this,&AZombieSpawnManager::MaintainZombiePopulation,SpawnInterval,true,PreparationDuration);
	GetWorldTimerManager().SetTimer(InitialSuppliesTimer,this,&AZombieSpawnManager::SpawnInitialSupplies,1.25f,false);
	GetWorldTimerManager().SetTimer(AmmoTimer,this,&AZombieSpawnManager::TrySpawnAmmunition,AmmoSpawnInterval,true,7.f);
	GetWorldTimerManager().SetTimer(HealthTimer,this,&AZombieSpawnManager::TrySpawnHealth,HealthSpawnInterval,true,8.f);
}

float AZombieSpawnManager::GetPreparationRemaining()const
{
	if(!GetWorld())return 0.f;
	const AGameStateBase* State=GetWorld()->GetGameState();
	const float Now=State?State->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();
	return FMath::Max(0.f,PreparationEndServerTime-Now);
}
void AZombieSpawnManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AZombieSpawnManager,PreparationEndServerTime);}

AShooterCharacter* AZombieSpawnManager::FindPlayer()const
{
	for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)if(!It->IsDead())return *It;
	return nullptr;
}

int32 AZombieSpawnManager::GetHighestPlayerLevel()const
{
	int32 HighestLevel=1;
	for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)
		if(!It->IsDead())HighestLevel=FMath::Max(HighestLevel,It->CharacterLevel);
	return HighestLevel;
}

bool AZombieSpawnManager::FindGroundedLocation(const FVector& Around,float MinDistance,float MaxDistance,FVector& OutLocation)const
{
	for(int32 Attempt=0;Attempt<12;++Attempt)
	{
		const float Angle=FMath::FRandRange(0.f,2.f*PI);
		const float Distance=FMath::FRandRange(MinDistance,MaxDistance);
		FVector Candidate=Around+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0.f)*Distance;
		FHitResult GroundHit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(DynamicSpawnGround),false,this);
		const FVector TraceStart=Candidate+FVector(0.f,0.f,2000.f);
		const FVector TraceEnd=Candidate-FVector(0.f,0.f,3000.f);
		if(GetWorld()->LineTraceSingleByChannel(GroundHit,TraceStart,TraceEnd,ECC_WorldStatic,Query))
		{
			OutLocation=GroundHit.ImpactPoint;
			return true;
		}
	}
	return false;
}

void AZombieSpawnManager::MaintainZombiePopulation()
{
	AShooterCharacter* Player=FindPlayer();
	if(!Player)return;
	int32 LivingZombies=0,LivingRunners=0,LivingBrutes=0,LivingSpitters=0;
	for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It)if(!It->IsDead())
	{
		++LivingZombies;
		if(It->IsA<ABruteZombieCharacter>())++LivingBrutes;
		else if(It->IsA<ASpitterZombieCharacter>())++LivingSpitters;
		else if(It->IsA<ARunnerZombieCharacter>())++LivingRunners;
	}
	const int32 HighestPlayerLevel=GetHighestPlayerLevel();
	const int32 TargetPopulation=FMath::Clamp(MaxAliveZombies+FMath::Max(0,HighestPlayerLevel-1)*ZombiesPerPlayerLevel,MaxAliveZombies,AbsoluteMaxAliveZombies);
	const int32 ToSpawn=FMath::Min(SpawnBatchSize,FMath::Max(0,TargetPopulation-LivingZombies));
	const int32 FiveLevelMilestones=HighestPlayerLevel/5;
	const float RunnerChance=FiveLevelMilestones>0?FMath::Clamp(.4f+(FiveLevelMilestones-1)*.05f,.4f,.75f):0.f;
	const int32 TenLevelMilestones=HighestPlayerLevel/10;
	const float BruteChance=TenLevelMilestones>0?FMath::Clamp(.15f+(TenLevelMilestones-1)*.04f,.15f,.45f):0.f;
	const int32 MinimumBrutes=FMath::Clamp(TenLevelMilestones,0,3);
	const int32 MinimumRunners=FiveLevelMilestones>0?1:0;
	const int32 FifteenLevelMilestones=HighestPlayerLevel/15;
	const float SpitterChance=FifteenLevelMilestones>0?FMath::Clamp(.18f+(FifteenLevelMilestones-1)*.04f,.18f,.45f):0.f;
	const int32 MinimumSpitters=FMath::Clamp(FifteenLevelMilestones,0,3);
	for(int32 Index=0;Index<ToSpawn;++Index)
	{
		FVector SpawnLocation;
		if(!FindGroundedLocation(Player->GetActorLocation(),MinimumSpawnDistance,MaximumSpawnDistance,SpawnLocation))continue;
		SpawnLocation.Z+=95.f;
		FVector ToPlayer=Player->GetActorLocation()-SpawnLocation;ToPlayer.Z=0.f;
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const bool bSpawnBrute=LivingBrutes<MinimumBrutes||FMath::FRand()<BruteChance;
		const bool bSpawnSpitter=!bSpawnBrute&&(LivingSpitters<MinimumSpitters||FMath::FRand()<SpitterChance);
		const bool bSpawnRunner=!bSpawnBrute&&!bSpawnSpitter&&(LivingRunners<MinimumRunners||FMath::FRand()<RunnerChance);
		TSubclassOf<AZombieCharacter> ZombieClass=bSpawnBrute?ABruteZombieCharacter::StaticClass():(bSpawnSpitter?ASpitterZombieCharacter::StaticClass():(bSpawnRunner?ARunnerZombieCharacter::StaticClass():AZombieCharacter::StaticClass()));
		AZombieCharacter* Zombie=GetWorld()->SpawnActor<AZombieCharacter>(ZombieClass,SpawnLocation,ToPlayer.Rotation(),Parameters);
		if(Zombie)
		{
			if(bSpawnBrute)++LivingBrutes;
			else if(bSpawnSpitter)++LivingSpitters;
			else if(bSpawnRunner)++LivingRunners;
			if(!Zombie->GetController())Zombie->SpawnDefaultController();
			if(UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld()))
				if(Zombie->NavigationInvoker)Zombie->NavigationInvoker->RegisterWithNavigationSystem(*Navigation);
			UE_LOG(LogTemp,Display,TEXT("Spawned %s zombie at %s (%d/%d alive, player level %d), controller=%s"),
				bSpawnBrute?TEXT("brute"):(bSpawnSpitter?TEXT("spitter"):(bSpawnRunner?TEXT("runner"):TEXT("roaming"))),*SpawnLocation.ToCompactString(),LivingZombies+Index+1,TargetPopulation,
				HighestPlayerLevel,*GetNameSafe(Zombie->GetController()));
		}
	}
}

void AZombieSpawnManager::TrySpawnAmmunition()
{
	if(FMath::FRand()>AmmoSpawnChance)return;
	SpawnAmmunition();
}

void AZombieSpawnManager::TrySpawnHealth()
{
	if(FMath::FRand()>HealthSpawnChance)return;
	SpawnHealth();
}

void AZombieSpawnManager::SpawnInitialSupplies()
{
	SpawnAmmunition();
	SpawnHealth();
}

bool AZombieSpawnManager::SpawnAmmunition()
{
	AShooterCharacter* Player=FindPlayer();
	if(!Player)return false;
	int32 PickupCount=0;
	for(TActorIterator<AAmmoPickup> It(GetWorld());It;++It)++PickupCount;
	if(PickupCount>=MaxAmmoPickups)return false;
	FVector SpawnLocation;
	if(!FindGroundedLocation(Player->GetActorLocation(),MinimumSupplyDistance,MaximumSupplyDistance,SpawnLocation))return false;
	SpawnLocation.Z+=35.f;
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	if(GetWorld()->SpawnActor<AAmmoPickup>(AAmmoPickup::StaticClass(),SpawnLocation,FRotator::ZeroRotator,Parameters))
	{
		UE_LOG(LogTemp,Display,TEXT("Spawned ammunition pickup at %s"),*SpawnLocation.ToCompactString());
		return true;
	}
	return false;
}

bool AZombieSpawnManager::SpawnHealth()
{
	AShooterCharacter* Player=FindPlayer();
	if(!Player)return false;
	int32 PickupCount=0;
	for(TActorIterator<AHealthPickup> It(GetWorld());It;++It)++PickupCount;
	if(PickupCount>=MaxHealthPickups)return false;
	FVector SpawnLocation;
	if(!FindGroundedLocation(Player->GetActorLocation(),MinimumSupplyDistance,MaximumSupplyDistance,SpawnLocation))return false;
	SpawnLocation.Z+=35.f;
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	if(GetWorld()->SpawnActor<AHealthPickup>(AHealthPickup::StaticClass(),SpawnLocation,FRotator::ZeroRotator,Parameters))
	{
		UE_LOG(LogTemp,Display,TEXT("Spawned health pickup at %s"),*SpawnLocation.ToCompactString());
		return true;
	}
	return false;
}
