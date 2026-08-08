#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OpenWorldTile.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UBoxComponent;

UCLASS()
class MYPROJECT_API AOpenWorldTile : public AActor
{
	GENERATED_BODY()
public:
	AOpenWorldTile();
	virtual void OnConstruction(const FTransform& Transform)override;
	virtual void BeginPlay()override;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)USceneComponent* SceneRoot;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Ground;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UBoxComponent* GroundCollision;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* Rocks;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* Bushes;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="1000.0"))float TileSize=20000.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="0"))int32 RockCount=10;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="0"))int32 BushCount=28;
private:
	void RebuildDecorations();
};
