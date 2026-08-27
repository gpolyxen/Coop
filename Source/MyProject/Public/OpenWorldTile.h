#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OpenWorldTile.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UBoxComponent;
class AHarvestableTree;
class UProceduralMeshComponent;
class UPostProcessComponent;

UCLASS()
class MYPROJECT_API AOpenWorldTile : public AActor
{
	GENERATED_BODY()
public:
	AOpenWorldTile();
	virtual void OnConstruction(const FTransform& Transform)override;
	virtual void BeginPlay()override;
	virtual void Tick(float DeltaSeconds)override;
	UFUNCTION(BlueprintPure,Category="Open World")float GetHeightAtWorldLocation(const FVector& WorldLocation)const{return GetTerrainHeight(WorldLocation.X,WorldLocation.Y)+GetActorLocation().Z;}

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)USceneComponent* SceneRoot;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Ground;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UProceduralMeshComponent* Terrain;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* GroundTiles;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UBoxComponent* GroundCollision;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* Rocks;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* RockCollision;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* Bushes;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* GrassA;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* GrassB;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* WaterBodies;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* RiverSurface;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* RiverUnderSurface;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UProceduralMeshComponent* RiverMesh;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UBoxComponent* RiverVolume;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UPostProcessComponent* UnderwaterPostProcess;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* LakeSurfaceA;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* LakeSurfaceB;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* LakeSurfaceC;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* Hills;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHierarchicalInstancedStaticMeshComponent* HillCollision;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="1000.0"))float TileSize=20000.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="0"))int32 RockCount=10;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="0"))int32 BushCount=28;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="0"))int32 GrassCount=180;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="0"))int32 TreeCount=18;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="250.0"))float GroundVisualCellSize=1000.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="0.0",ClampMax="1.0"))float LakeChance=.22f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="0"))int32 HillCount=10;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="8",ClampMax="128"))int32 TerrainResolution=80;
private:
	void RebuildDecorations();
	void RebuildGroundTiles();
	void RebuildWaterBodies();
	void SpawnHarvestableTrees();
	UPROPERTY(Transient)TArray<AHarvestableTree*> SpawnedTrees;
	float GetTerrainHeight(float WorldX,float WorldY)const;
};
