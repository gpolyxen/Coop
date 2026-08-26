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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	UFUNCTION(BlueprintPure)float GetPreparationRemaining()const;
	UFUNCTION(BlueprintPure)bool IsPreparing()const{return GetPreparationRemaining()>0.f;}
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="0"))float PreparationDuration=60.f;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Zombie Spawning")float PreparationEndServerTime=0.f;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="1"))int32 MaxAliveZombies=8;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="0"))int32 ZombiesPerPlayerLevel=1;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="8"))int32 AbsoluteMaxAliveZombies=40;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="1"))int32 SpawnBatchSize=2;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="0.5"))float SpawnInterval=4.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="300"))float MinimumSpawnDistance=1300.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Zombie Spawning",meta=(ClampMin="500"))float MaximumSpawnDistance=3000.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Supplies",meta=(ClampMin="100"))float MinimumSupplyDistance=350.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Supplies",meta=(ClampMin="300"))float MaximumSupplyDistance=1200.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Spawning",meta=(ClampMin="100"))float ConstructionExclusionRadius=550.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ammunition",meta=(ClampMin="0.0",ClampMax="1.0"))float AmmoSpawnChance=.75f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ammunition",meta=(ClampMin="1"))int32 MaxAmmoPickups=6;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ammunition",meta=(ClampMin="2.0"))float AmmoSpawnInterval=8.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Health",meta=(ClampMin="0.0",ClampMax="1.0"))float HealthSpawnChance=.65f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Health",meta=(ClampMin="1"))int32 MaxHealthPickups=4;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Health",meta=(ClampMin="2.0"))float HealthSpawnInterval=10.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Bandits",meta=(ClampMin="1"))int32 BanditUnlockLevel=20;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Bandits",meta=(ClampMin="1"))int32 InitialBanditGroupSize=3;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Bandits",meta=(ClampMin="10.0"))float BanditGroupCooldown=90.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Bandits",meta=(ClampMin="500"))float MinimumBanditSpawnDistance=2600.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Bandits",meta=(ClampMin="1000"))float MaximumBanditSpawnDistance=4300.f;

private:
	void MaintainZombiePopulation();
	void SpawnInitialSupplies();
	void TrySpawnAmmunition();
	void TrySpawnHealth();
	bool SpawnAmmunition();
	bool SpawnHealth();
	bool FindGroundedLocation(const FVector& Around,float MinDistance,float MaxDistance,FVector& OutLocation)const;
	bool IsNearPlayerConstruction(const FVector& Location)const;
	AShooterCharacter* FindPlayer()const;
	int32 GetHighestPlayerLevel()const;
	void MaintainBanditPopulation(int32 HighestPlayerLevel,AShooterCharacter* NearPlayer);
	FTimerHandle ZombieTimer;
	FTimerHandle AmmoTimer;
	FTimerHandle HealthTimer;
	FTimerHandle InitialSuppliesTimer;
	double NextBanditGroupTime=0.;
};
