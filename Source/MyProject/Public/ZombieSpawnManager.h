#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieSpawnManager.generated.h"

class AShooterCharacter;

UCLASS(Blueprintable)
class MYPROJECT_API AZombieSpawnManager : public AActor
{
	GENERATED_BODY()
public:
	AZombieSpawnManager();
	virtual void BeginPlay()override;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="1"))int32 MaxAliveZombies=8;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="1"))int32 SpawnBatchSize=2;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="0.5"))float SpawnInterval=4.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="300"))float MinimumSpawnDistance=1300.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="500"))float MaximumSpawnDistance=3000.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Supplies",meta=(ClampMin="100"))float MinimumSupplyDistance=350.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Supplies",meta=(ClampMin="300"))float MaximumSupplyDistance=1200.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ammunition",meta=(ClampMin="0.0",ClampMax="1.0"))float AmmoSpawnChance=.75f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ammunition",meta=(ClampMin="1"))int32 MaxAmmoPickups=6;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ammunition",meta=(ClampMin="2.0"))float AmmoSpawnInterval=8.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Health",meta=(ClampMin="0.0",ClampMax="1.0"))float HealthSpawnChance=.65f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Health",meta=(ClampMin="1"))int32 MaxHealthPickups=4;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Health",meta=(ClampMin="2.0"))float HealthSpawnInterval=10.f;

private:
	void MaintainZombiePopulation();
	void SpawnInitialSupplies();
	void TrySpawnAmmunition();
	void TrySpawnHealth();
	bool SpawnAmmunition();
	bool SpawnHealth();
	bool FindGroundedLocation(const FVector& Around,float MinDistance,float MaxDistance,FVector& OutLocation)const;
	AShooterCharacter* FindPlayer()const;
	FTimerHandle ZombieTimer;
	FTimerHandle AmmoTimer;
	FTimerHandle HealthTimer;
	FTimerHandle InitialSuppliesTimer;
};
