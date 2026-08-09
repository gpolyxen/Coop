#include "ZombieSpawnManager.h"

#include "AmmoPickup.h"
#include "HealthPickup.h"
#include "ShooterCharacter.h"
#include "ZombieCharacter.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"

AZombieSpawnManager::AZombieSpawnManager()
{
	PrimaryActorTick.bCanEverTick=false;
	bReplicates=false;
}

void AZombieSpawnManager::BeginPlay()
{
	Super::BeginPlay();
	if(!HasAuthority())return;
	GetWorldTimerManager().SetTimer(ZombieTimer,this,&AZombieSpawnManager::MaintainZombiePopulation,SpawnInterval,true,.75f);
	GetWorldTimerManager().SetTimer(InitialSuppliesTimer,this,&AZombieSpawnManager::SpawnInitialSupplies,1.25f,false);
	GetWorldTimerManager().SetTimer(AmmoTimer,this,&AZombieSpawnManager::TrySpawnAmmunition,AmmoSpawnInterval,true,7.f);
	GetWorldTimerManager().SetTimer(HealthTimer,this,&AZombieSpawnManager::TrySpawnHealth,HealthSpawnInterval,true,8.f);
}

AShooterCharacter* AZombieSpawnManager::FindPlayer()const
{
	for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)if(!It->IsDead())return *It;
	return nullptr;
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
	int32 LivingZombies=0;
	for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It)if(!It->IsDead())++LivingZombies;
	const int32 ToSpawn=FMath::Min(SpawnBatchSize,FMath::Max(0,MaxAliveZombies-LivingZombies));
	for(int32 Index=0;Index<ToSpawn;++Index)
	{
		FVector SpawnLocation;
		if(!FindGroundedLocation(Player->GetActorLocation(),MinimumSpawnDistance,MaximumSpawnDistance,SpawnLocation))continue;
		SpawnLocation.Z+=95.f;
		FVector ToPlayer=Player->GetActorLocation()-SpawnLocation;ToPlayer.Z=0.f;
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AZombieCharacter* Zombie=GetWorld()->SpawnActor<AZombieCharacter>(AZombieCharacter::StaticClass(),SpawnLocation,ToPlayer.Rotation(),Parameters);
		if(Zombie)UE_LOG(LogTemp,Display,TEXT("Spawned roaming zombie at %s (%d/%d alive)"),*SpawnLocation.ToCompactString(),LivingZombies+Index+1,MaxAliveZombies);
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
