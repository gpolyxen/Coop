#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OpenWorldStreamingManager.generated.h"

class ULevelStreamingDynamic;
class AOpenWorldTile;
class USceneComponent;
class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;

UCLASS()
class MYPROJECT_API AOpenWorldStreamingManager : public AActor
{
	GENERATED_BODY()
public:
	AOpenWorldStreamingManager();
	virtual void BeginPlay()override;
	virtual void Tick(float DeltaSeconds)override;
	void PrepareStartingTile(const FVector& AbsoluteLocation);
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)USceneComponent* SceneRoot;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Environment")UDirectionalLightComponent* Sun;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Environment")USkyAtmosphereComponent* Atmosphere;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Environment")USkyLightComponent* SkyLight;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Environment")UExponentialHeightFogComponent* HeightFog;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World")FString TileLevelPackage=TEXT("/Game/OpenWorld/Tiles/OpenWorldTile");
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="1000.0"))float TileSize=20000.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="1"))int32 LoadRadiusInTiles=1;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="2"))int32 UnloadRadiusInTiles=2;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="1"))int32 WorldRadiusInTiles=16;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Open World",meta=(ClampMin="10000.0"))float OriginRebaseDistance=200000.f;
private:
	UPROPERTY(Transient)TMap<FIntPoint,ULevelStreamingDynamic*> ActiveTiles;
	UPROPERTY(Transient)TMap<FIntPoint,AOpenWorldTile*> TileGeometry;
	UPROPERTY(Transient)AOpenWorldTile* BootstrapTile=nullptr;
	FIntPoint BootstrapCoordinate=FIntPoint::ZeroValue;
	bool bBootstrapCoordinateSet=false;
	void UpdateStreaming();
	void EnsureBootstrapTile();
	void CreateGeometryForLoadedTiles();
	void MaybeRebaseOrigin();
	void GetRelevantAbsoluteLocations(TArray<FVector>& OutLocations)const;
	void LoadTile(const FIntPoint& Coordinate);
	void UnloadTile(const FIntPoint& Coordinate);
	FIntPoint LocationToTile(const FVector& AbsoluteLocation)const;
	FVector TileToLocalLocation(const FIntPoint& Coordinate)const;
	FString MakeInstanceName(const FIntPoint& Coordinate)const;
};
