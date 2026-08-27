#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HarvestableTree.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class MYPROJECT_API AHarvestableTree : public AActor
{
	GENERATED_BODY()
public:
	AHarvestableTree();
	virtual void OnConstruction(const FTransform& Transform)override;
	virtual void BeginPlay()override;
	virtual void Tick(float DeltaSeconds)override;
	virtual float TakeDamage(float DamageAmount,const FDamageEvent& DamageEvent,AController* EventInstigator,AActor* DamageCauser)override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)USceneComponent* SceneRoot;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Trunk;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Crown;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* CrownLeft;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* CrownRight;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Harvest",meta=(ClampMin="1"))int32 RequiredAxeHits=10;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Harvest",meta=(ClampMin="1"))int32 WoodReward=6;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Harvest")int32 AxeHits=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Harvest")bool bFalling=false;
private:void ApplyMaterials();
	float FallElapsed=0.f;
	float FallRollSign=1.f;
};
