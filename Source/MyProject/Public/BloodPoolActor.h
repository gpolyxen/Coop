#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BloodPoolActor.generated.h"

class UStaticMeshComponent;
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
	UPROPERTY()UStaticMeshComponent* PoolMesh=nullptr;
	TWeakObjectPtr<USkeletalMeshComponent> TrackedCorpseMesh;
	FVector TargetScale=FVector(1.5f,1.5f,.018f);
	float Age=0.f;
	void MoveBelowCorpse(const FVector& CorpseLocation);
};
