#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BloodPoolActor.generated.h"

class USceneComponent;
class UDecalComponent;
class USkeletalMeshComponent;

UCLASS(NotBlueprintable)
class MYPROJECT_API ABloodPoolActor : public AActor
{
	GENERATED_BODY()
public:
	ABloodPoolActor();
	virtual void Tick(float DeltaSeconds)override;
	void ActivatePool(USkeletalMeshComponent* CorpseMesh,const FVector& CorpseLocation);
private:
	UPROPERTY()USceneComponent* SceneRoot=nullptr;
	UPROPERTY()TArray<UDecalComponent*> PoolDecals;
	TArray<FVector> TargetDecalSizes;
	TWeakObjectPtr<USkeletalMeshComponent> TrackedCorpseMesh;
	float Age=0.f;
	void MoveBelowCorpse(const FVector& CorpseLocation);
};
