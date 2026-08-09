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
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ammunition",meta=(ClampMin="0.0",ClampMax="1.0"))float AmmoSpawnChance=.35f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ammunition",meta=(ClampMin="1"))int32 MaxAmmoPickups=5;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ammunition",meta=(ClampMin="2.0"))float AmmoSpawnInterval=12.f;

private:
	void MaintainZombiePopulation();
	void TrySpawnAmmunition();
	bool FindGroundedLocation(const FVector& Around,float MinDistance,float MaxDistance,FVector& OutLocation)const;
	AShooterCharacter* FindPlayer()const;
	FTimerHandle ZombieTimer;
	FTimerHandle AmmoTimer;
};
