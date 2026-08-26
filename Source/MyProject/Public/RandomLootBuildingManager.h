#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RandomLootBuildingManager.generated.h"

/** Generates a few small destructible ruins on a fresh world. */
UCLASS()
class MYPROJECT_API ARandomLootBuildingManager : public AActor
{
	GENERATED_BODY()
public:
	ARandomLootBuildingManager();
	virtual void BeginPlay()override;
	UPROPERTY(EditAnywhere,Category="Generated Buildings",meta=(ClampMin="1",ClampMax="32"))int32 BuildingCount=12;
	UPROPERTY(EditAnywhere,Category="Generated Buildings",meta=(ClampMin="1",ClampMax="12"))int32 ZombiesPerBuilding=4;
	UPROPERTY(EditAnywhere,Category="Generated Buildings")FVector GenerationCenter=FVector::ZeroVector;
	UPROPERTY(EditAnywhere,Category="Generated Buildings",meta=(ClampMin="1000"))float MinimumGenerationRadius=3000.f;
	UPROPERTY(EditAnywhere,Category="Generated Buildings",meta=(ClampMin="2000"))float MaximumGenerationRadius=12000.f;
	UPROPERTY(EditAnywhere,Category="Generated Buildings",meta=(ClampMin="600"))float MinimumBuildingSpacing=1250.f;
	UPROPERTY(EditAnywhere,Category="Generated Buildings",meta=(ClampMin="1",ClampMax="6"))int32 MaximumFloors=4;
private:
	void Generate();
	bool FindGround(const FVector& Candidate,FVector& OutGround)const;
	void SpawnBuilding(const FVector& GroundLocation,float Yaw,int32 BuildingIndex,int32 FloorCount,int32 WidthModules);
	FTimerHandle GenerationTimer;
};
