#include "OpenWorldStreamingManager.h"
#include "OpenWorldTile.h"
#include "ShooterCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Net/UnrealNetwork.h"

AOpenWorldStreamingManager::AOpenWorldStreamingManager()
{
	PrimaryActorTick.bCanEverTick=true;
	PrimaryActorTick.TickInterval=.5f;
	bReplicates=true;
	bAlwaysRelevant=true;
	NetUpdateFrequency=2.f;
	SetReplicateMovement(false);
	SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent=SceneRoot;
	Sun=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(SceneRoot);
	Sun->SetRelativeRotation(FRotator(-40.f,-35.f,0.f));
	Sun->SetIntensity(6.f);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetAtmosphereSunLight(true);
	Atmosphere=CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	Atmosphere->SetupAttachment(SceneRoot);
	SkyLight=CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(SceneRoot);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetIntensity(1.f);
	SkyLight->bRealTimeCapture=true;
	HeightFog=CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(SceneRoot);
	HeightFog->SetFogDensity(.003f);
}

void AOpenWorldStreamingManager::BeginPlay()
{
	Super::BeginPlay();
	if(HasAuthority())
	{
		CurrentTimeHours=FMath::Fmod(FMath::Max(0.f,StartingTimeHours),24.f);
		bIsNight=CurrentTimeHours>=21.f||CurrentTimeHours<5.f;
	}
	ApplyTimeOfDay();
	if(AWorldSettings* Settings=GetWorld()->GetWorldSettings())Settings->bEnableWorldBoundsChecks=false;
	EnsureBootstrapTile();
	UpdateStreaming();
}

void AOpenWorldStreamingManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if(HasAuthority())
	{
		CurrentTimeHours=FMath::Fmod(CurrentTimeHours+DeltaSeconds*24.f/FMath::Max(60.f,RealSecondsPerGameDay),24.f);
		const bool bNewNight=CurrentTimeHours>=21.f||CurrentTimeHours<5.f;
		if(bNewNight!=bIsNight){bIsNight=bNewNight;ForceNetUpdate();}
	}
	ApplyTimeOfDay();
	MaybeRebaseOrigin();
	EnsureBootstrapTile();
	UpdateStreaming();
	CreateGeometryForLoadedTiles();
}

void AOpenWorldStreamingManager::OnRep_TimeOfDay(){ApplyTimeOfDay();}

FText AOpenWorldStreamingManager::GetTimePeriodName()const
{
	if(CurrentTimeHours>=5.f&&CurrentTimeHours<10.f)return FText::FromString(TEXT("УТРО"));
	if(CurrentTimeHours>=10.f&&CurrentTimeHours<17.f)return FText::FromString(TEXT("ДЕНЬ"));
	if(CurrentTimeHours>=17.f&&CurrentTimeHours<21.f)return FText::FromString(TEXT("ВЕЧЕР"));
	return FText::FromString(TEXT("НОЧЬ"));
}

void AOpenWorldStreamingManager::ApplyTimeOfDay()
{
	if(!Sun||!SkyLight||!HeightFog)return;
	const float SolarRadians=(CurrentTimeHours-6.f)/12.f*PI;
	const float SunHeight=FMath::Sin(SolarRadians);
	const float DayFactor=FMath::Clamp(SunHeight/.18f,0.f,1.f);
	Sun->SetWorldRotation(FRotator(-(CurrentTimeHours-6.f)*15.f,-35.f,0.f));
	Sun->SetIntensity(FMath::Lerp(.025f,6.f,DayFactor));
	FLinearColor SunColor=FLinearColor(.48f,.58f,1.f);
	if(DayFactor>.01f)
	{
		const float HorizonWarmth=1.f-FMath::Clamp(SunHeight/.45f,0.f,1.f);
		SunColor=FLinearColor::LerpUsingHSV(FLinearColor(1.f,.38f,.14f),FLinearColor(1.f,.96f,.86f),1.f-HorizonWarmth);
	}
	Sun->SetLightColor(SunColor);
	SkyLight->SetIntensity(FMath::Lerp(.09f,1.f,DayFactor));
	HeightFog->SetFogDensity(FMath::Lerp(.012f,.003f,DayFactor));
	HeightFog->SetFogInscatteringColor(FLinearColor::LerpUsingHSV(FLinearColor(.025f,.045f,.11f),FLinearColor(.64f,.72f,.78f),DayFactor));
}

void AOpenWorldStreamingManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOpenWorldStreamingManager,CurrentTimeHours);
	DOREPLIFETIME(AOpenWorldStreamingManager,bIsNight);
}

void AOpenWorldStreamingManager::EnsureBootstrapTile()
{
	if(BootstrapTile||bBootstrapCoordinateSet)return;
	TArray<FVector> PlayerLocations;
	GetRelevantAbsoluteLocations(PlayerLocations);
	if(PlayerLocations.Num()==0)return;
	PrepareStartingTile(PlayerLocations[0]);
}

void AOpenWorldStreamingManager::PrepareStartingTile(const FVector& AbsoluteLocation)
{
	if(BootstrapTile||bBootstrapCoordinateSet)return;

	BootstrapCoordinate=LocationToTile(AbsoluteLocation);
	bBootstrapCoordinateSet=true;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	BootstrapTile=GetWorld()->SpawnActor<AOpenWorldTile>(AOpenWorldTile::StaticClass(),TileToLocalLocation(BootstrapCoordinate),FRotator::ZeroRotator,SpawnParameters);
	if(!BootstrapTile)
	{
		bBootstrapCoordinateSet=false;
		UE_LOG(LogTemp,Error,TEXT("Could not create synchronous open-world bootstrap tile"));
		return;
	}
	TileGeometry.Add(BootstrapCoordinate,BootstrapTile);
	UE_LOG(LogTemp,Display,TEXT("Created synchronous open-world bootstrap tile (%d,%d)"),BootstrapCoordinate.X,BootstrapCoordinate.Y);

	// A pawn may already exist when GameMode creates this manager. If a slow disk
	// allowed it to start falling, put it back above the newly created collision.
	const FVector OriginOffset(GetWorld()->OriginLocation);
	for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)
	{
		if(LocationToTile(It->GetActorLocation()+OriginOffset)!=BootstrapCoordinate||It->GetActorLocation().Z>=100.f)continue;
		FVector SafeLocation=It->GetActorLocation();
		SafeLocation.Z=200.f;
		It->SetActorLocation(SafeLocation,false,nullptr,ETeleportType::TeleportPhysics);
		if(UCharacterMovementComponent* Movement=It->GetCharacterMovement())Movement->StopMovementImmediately();
	}
}

void AOpenWorldStreamingManager::GetRelevantAbsoluteLocations(TArray<FVector>& OutLocations)const
{
	const FVector OriginOffset(GetWorld()->OriginLocation);
	if(GetNetMode()==NM_Client)
	{
		for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
		{
			const APlayerController* Controller=It->Get();
			if(Controller&&Controller->IsLocalController()&&Controller->GetPawn())OutLocations.Add(Controller->GetPawn()->GetActorLocation()+OriginOffset);
		}
		return;
	}
	for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)OutLocations.Add(It->GetActorLocation()+OriginOffset);
}

FIntPoint AOpenWorldStreamingManager::LocationToTile(const FVector& AbsoluteLocation)const
{
	return FIntPoint(FMath::RoundToInt(AbsoluteLocation.X/TileSize),FMath::RoundToInt(AbsoluteLocation.Y/TileSize));
}

FVector AOpenWorldStreamingManager::TileToLocalLocation(const FIntPoint& Coordinate)const
{
	return FVector(Coordinate.X*TileSize,Coordinate.Y*TileSize,0.f)-FVector(GetWorld()->OriginLocation);
}

FString AOpenWorldStreamingManager::MakeInstanceName(const FIntPoint& Coordinate)const
{
	const FString X=Coordinate.X<0?FString::Printf(TEXT("N%d"),-Coordinate.X):FString::Printf(TEXT("P%d"),Coordinate.X);
	const FString Y=Coordinate.Y<0?FString::Printf(TEXT("N%d"),-Coordinate.Y):FString::Printf(TEXT("P%d"),Coordinate.Y);
	return FString::Printf(TEXT("OpenWorld_X%s_Y%s"),*X,*Y);
}

void AOpenWorldStreamingManager::LoadTile(const FIntPoint& Coordinate)
{
	if(ActiveTiles.Contains(Coordinate))return;
	bool bLoaded=false;
	ULevelStreamingDynamic* Streaming=ULevelStreamingDynamic::LoadLevelInstance(this,TileLevelPackage,TileToLocalLocation(Coordinate),FRotator::ZeroRotator,bLoaded,MakeInstanceName(Coordinate));
	if(bLoaded&&Streaming)ActiveTiles.Add(Coordinate,Streaming);
	else UE_LOG(LogTemp,Error,TEXT("Could not stream open-world tile %s for (%d,%d)"),*TileLevelPackage,Coordinate.X,Coordinate.Y);
}

void AOpenWorldStreamingManager::UnloadTile(const FIntPoint& Coordinate)
{
	if(ULevelStreamingDynamic** Found=ActiveTiles.Find(Coordinate))
	{
		if(ULevelStreamingDynamic* Streaming=*Found)
		{
			Streaming->SetShouldBeVisible(false);
			Streaming->SetShouldBeLoaded(false);
			Streaming->SetIsRequestingUnloadAndRemoval(true);
		}
	}
	TileGeometry.Remove(Coordinate);
	ActiveTiles.Remove(Coordinate);
}

void AOpenWorldStreamingManager::CreateGeometryForLoadedTiles()
{
	for(const TPair<FIntPoint,ULevelStreamingDynamic*>& Pair:ActiveTiles)
	{
		if(!Pair.Value||!Pair.Value->IsLevelLoaded()||!Pair.Value->GetLoadedLevel())continue;
		const bool bReplacingBootstrap=BootstrapTile&&bBootstrapCoordinateSet&&Pair.Key==BootstrapCoordinate;
		if(TileGeometry.Contains(Pair.Key)&&!bReplacingBootstrap)continue;
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel=Pair.Value->GetLoadedLevel();
		SpawnParameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AOpenWorldTile* Geometry=GetWorld()->SpawnActor<AOpenWorldTile>(AOpenWorldTile::StaticClass(),TileToLocalLocation(Pair.Key),FRotator::ZeroRotator,SpawnParameters);
		if(Geometry)
		{
			TileGeometry.Add(Pair.Key,Geometry);
			if(bReplacingBootstrap)
			{
				AOpenWorldTile* TileToDestroy=BootstrapTile;
				BootstrapTile=nullptr;
				TileToDestroy->Destroy();
				UE_LOG(LogTemp,Display,TEXT("Replaced bootstrap tile (%d,%d) with streamed geometry"),Pair.Key.X,Pair.Key.Y);
			}
		}
	}
}

void AOpenWorldStreamingManager::UpdateStreaming()
{
	TArray<FVector> PlayerLocations;
	GetRelevantAbsoluteLocations(PlayerLocations);
	if(PlayerLocations.Num()==0)return;
	TSet<FIntPoint> NeededTiles;
	for(const FVector& Location:PlayerLocations)
	{
		const FIntPoint Center=LocationToTile(Location);
		for(int32 X=-LoadRadiusInTiles;X<=LoadRadiusInTiles;++X)
			for(int32 Y=-LoadRadiusInTiles;Y<=LoadRadiusInTiles;++Y)
			{
				const FIntPoint Coordinate=Center+FIntPoint(X,Y);
				if(FMath::Abs(Coordinate.X)<=WorldRadiusInTiles&&FMath::Abs(Coordinate.Y)<=WorldRadiusInTiles)NeededTiles.Add(Coordinate);
			}
	}
	for(const FIntPoint& Coordinate:NeededTiles)LoadTile(Coordinate);

	TArray<FIntPoint> ToUnload;
	for(const TPair<FIntPoint,ULevelStreamingDynamic*>& Pair:ActiveTiles)
	{
		bool bKeep=false;
		for(const FVector& Location:PlayerLocations)
		{
			const FIntPoint Center=LocationToTile(Location);
			if(FMath::Abs(Pair.Key.X-Center.X)<=UnloadRadiusInTiles&&FMath::Abs(Pair.Key.Y-Center.Y)<=UnloadRadiusInTiles){bKeep=true;break;}
		}
		if(!bKeep)ToUnload.Add(Pair.Key);
	}
	for(const FIntPoint& Coordinate:ToUnload)UnloadTile(Coordinate);
	CreateGeometryForLoadedTiles();
}

void AOpenWorldStreamingManager::MaybeRebaseOrigin()
{
	TArray<FVector> AbsoluteLocations;
	GetRelevantAbsoluteLocations(AbsoluteLocations);
	if(AbsoluteLocations.Num()==0)return;
	FVector LocalFocus=AbsoluteLocations[0]-FVector(GetWorld()->OriginLocation);
	if(FMath::Max(FMath::Abs(LocalFocus.X),FMath::Abs(LocalFocus.Y))<OriginRebaseDistance)return;
	const FIntVector Shift(FMath::RoundToInt(LocalFocus.X/TileSize)*FMath::RoundToInt(TileSize),FMath::RoundToInt(LocalFocus.Y/TileSize)*FMath::RoundToInt(TileSize),0);
	GetWorld()->RequestNewWorldOrigin(GetWorld()->OriginLocation+Shift);
}
