#include "OpenWorldTile.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"

AOpenWorldTile::AOpenWorldTile()
{
	PrimaryActorTick.bCanEverTick=false;
	bReplicates=false;
	SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent=SceneRoot;
	SceneRoot->SetMobility(EComponentMobility::Static);
	Ground=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ground"));
	Ground->SetupAttachment(SceneRoot);
	Ground->SetMobility(EComponentMobility::Static);
	Ground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ground->SetCanEverAffectNavigation(false);
	GroundCollision=CreateDefaultSubobject<UBoxComponent>(TEXT("GroundCollision"));
	GroundCollision->SetupAttachment(SceneRoot);
	GroundCollision->SetMobility(EComponentMobility::Static);
	GroundCollision->SetCollisionProfileName(TEXT("BlockAll"));
	GroundCollision->SetCanEverAffectNavigation(true);
	GroundCollision->SetHiddenInGame(true);
	Rocks=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Rocks"));
	Rocks->SetupAttachment(SceneRoot);
	Rocks->SetMobility(EComponentMobility::Static);
	Rocks->SetCollisionProfileName(TEXT("BlockAll"));
	Rocks->SetCanEverAffectNavigation(true);
	Bushes=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Bushes"));
	Bushes->SetupAttachment(SceneRoot);
	Bushes->SetMobility(EComponentMobility::Static);
	Bushes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bushes->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GroundMesh(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	if(GroundMesh.Succeeded())Ground->SetStaticMesh(GroundMesh.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GroundMaterial(TEXT("/Game/StarterContent/Materials/M_Ground_Grass.M_Ground_Grass"));
	if(GroundMaterial.Succeeded())Ground->SetMaterial(0,GroundMaterial.Object);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RockMesh(TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock"));
	if(RockMesh.Succeeded())Rocks->SetStaticMesh(RockMesh.Object);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BushMesh(TEXT("/Game/StarterContent/Props/SM_Bush.SM_Bush"));
	if(BushMesh.Succeeded())Bushes->SetStaticMesh(BushMesh.Object);
}

void AOpenWorldTile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Ground->SetRelativeLocation(FVector(0.f,0.f,-50.f));
	Ground->SetRelativeScale3D(FVector(TileSize/100.f,TileSize/100.f,1.f));
	GroundCollision->SetRelativeLocation(FVector::ZeroVector);
	GroundCollision->SetBoxExtent(FVector(TileSize*.5f,TileSize*.5f,50.f),false);
	RebuildDecorations();
}

void AOpenWorldTile::BeginPlay()
{
	Super::BeginPlay();
	// A streamed instance has its final level transform only at runtime, so rebuild
	// decoration from that position to keep every sector deterministic but different.
	RebuildDecorations();
	UNavigationSystemV1::UpdateActorInNavOctree(*this);
	if(UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		Navigation->AddDirtyArea(GetComponentsBoundingBox(true),ENavigationDirtyFlag::All);
	}
}

void AOpenWorldTile::RebuildDecorations()
{
	if(!Rocks||!Bushes)return;
	Rocks->ClearInstances();
	Bushes->ClearInstances();
	const FIntPoint Sector(FMath::RoundToInt(GetActorLocation().X/TileSize),FMath::RoundToInt(GetActorLocation().Y/TileSize));
	FRandomStream Random(HashCombine(GetTypeHash(Sector.X),GetTypeHash(Sector.Y)));
	for(int32 Index=0;Index<RockCount;++Index)
	{
		FVector Position(Random.FRandRange(-TileSize*.46f,TileSize*.46f),Random.FRandRange(-TileSize*.46f,TileSize*.46f),0.f);
		if(Position.SizeSquared2D()<FMath::Square(1200.f))Position.X+=FMath::Sign(Position.X==0.f?1.f:Position.X)*1400.f;
		const float Scale=Random.FRandRange(.7f,2.4f);
		const FRotator Rotation(0.f,Random.FRandRange(0.f,360.f),0.f);
		Rocks->AddInstance(FTransform(Rotation,Position,FVector(Scale)));
	}
	for(int32 Index=0;Index<BushCount;++Index)
	{
		FVector Position(Random.FRandRange(-TileSize*.47f,TileSize*.47f),Random.FRandRange(-TileSize*.47f,TileSize*.47f),0.f);
		if(Position.SizeSquared2D()<FMath::Square(1100.f))Position.Y+=FMath::Sign(Position.Y==0.f?1.f:Position.Y)*1300.f;
		const float Scale=Random.FRandRange(.65f,1.45f);
		const FRotator Rotation(0.f,Random.FRandRange(0.f,360.f),0.f);
		Bushes->AddInstance(FTransform(Rotation,Position,FVector(Scale)));
	}
}
