#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BloodBurstActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS(NotBlueprintable)
class MYPROJECT_API ABloodBurstActor : public AActor
{
	GENERATED_BODY()
public:
	ABloodBurstActor();
	virtual void Tick(float DeltaSeconds)override;
	void ActivateBurst(const FVector& ShotDirection,bool bFountain);
private:
	UPROPERTY()USceneComponent* SceneRoot=nullptr;
	UPROPERTY()TArray<UStaticMeshComponent*> Droplets;
	TArray<FVector> Velocities;
	float Age=0.f;
	bool bActive=false;
};
