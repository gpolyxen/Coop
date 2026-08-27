#include "OpenWorldTile.h"
#include "HarvestableTree.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AOpenWorldTile::AOpenWorldTile()
{
	PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.05f;
	bReplicates=false;
	SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent=SceneRoot;
	SceneRoot->SetMobility(EComponentMobility::Static);
	Ground=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ground"));
	Ground->SetupAttachment(SceneRoot);
	Ground->SetMobility(EComponentMobility::Static);
	Ground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ground->SetCanEverAffectNavigation(false);
	Ground->SetVisibility(false,true);
	Terrain=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Terrain"));Terrain->SetupAttachment(SceneRoot);Terrain->SetMobility(EComponentMobility::Static);Terrain->SetCollisionProfileName(TEXT("BlockAll"));Terrain->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Terrain->SetCanEverAffectNavigation(true);Terrain->bUseAsyncCooking=false;Terrain->bUseComplexAsSimpleCollision=true;
	GroundTiles=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GroundTiles"));GroundTiles->SetupAttachment(SceneRoot);GroundTiles->SetMobility(EComponentMobility::Static);GroundTiles->SetCollisionEnabled(ECollisionEnabled::NoCollision);GroundTiles->SetCanEverAffectNavigation(false);
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
	Rocks->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Rocks->SetCollisionObjectType(ECC_WorldStatic);
	Rocks->SetCollisionResponseToAllChannels(ECR_Block);
	Rocks->SetCanEverAffectNavigation(true);
	RockCollision=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RockCollision"));RockCollision->SetupAttachment(SceneRoot);RockCollision->SetMobility(EComponentMobility::Static);RockCollision->SetCollisionProfileName(TEXT("BlockAll"));RockCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);RockCollision->SetCollisionObjectType(ECC_WorldStatic);RockCollision->SetCollisionResponseToAllChannels(ECR_Block);RockCollision->SetCanEverAffectNavigation(true);RockCollision->SetVisibility(false,true);RockCollision->SetHiddenInGame(true);
	Bushes=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Bushes"));
	Bushes->SetupAttachment(SceneRoot);
	Bushes->SetMobility(EComponentMobility::Static);
	Bushes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bushes->SetCanEverAffectNavigation(false);
	GrassA=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassA"));GrassA->SetupAttachment(SceneRoot);GrassA->SetMobility(EComponentMobility::Movable);GrassA->SetCollisionEnabled(ECollisionEnabled::NoCollision);GrassA->SetCanEverAffectNavigation(false);GrassA->SetCastShadow(false);
	GrassB=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GrassB"));GrassB->SetupAttachment(SceneRoot);GrassB->SetMobility(EComponentMobility::Movable);GrassB->SetCollisionEnabled(ECollisionEnabled::NoCollision);GrassB->SetCanEverAffectNavigation(false);GrassB->SetCastShadow(false);
	WaterBodies=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WaterBodies"));WaterBodies->SetupAttachment(SceneRoot);WaterBodies->SetMobility(EComponentMobility::Static);WaterBodies->SetCollisionEnabled(ECollisionEnabled::NoCollision);WaterBodies->SetCanEverAffectNavigation(false);WaterBodies->SetCastShadow(false);
	RiverSurface=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RiverSurface"));RiverSurface->SetupAttachment(SceneRoot);
	RiverUnderSurface=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RiverUnderSurface"));RiverUnderSurface->SetupAttachment(SceneRoot);
	RiverMesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RiverMesh"));RiverMesh->SetupAttachment(SceneRoot);RiverMesh->SetMobility(EComponentMobility::Static);RiverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);RiverMesh->SetCanEverAffectNavigation(false);RiverMesh->SetCastShadow(false);
	LakeSurfaceA=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LakeSurfaceA"));LakeSurfaceA->SetupAttachment(SceneRoot);
	LakeSurfaceB=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LakeSurfaceB"));LakeSurfaceB->SetupAttachment(SceneRoot);
	LakeSurfaceC=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LakeSurfaceC"));LakeSurfaceC->SetupAttachment(SceneRoot);
	for(UStaticMeshComponent* WaterPart:{RiverSurface,RiverUnderSurface,LakeSurfaceA,LakeSurfaceB,LakeSurfaceC}){WaterPart->SetMobility(EComponentMobility::Static);WaterPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);WaterPart->SetCanEverAffectNavigation(false);WaterPart->SetCastShadow(false);}
	RiverVolume=CreateDefaultSubobject<UBoxComponent>(TEXT("RiverVolume"));RiverVolume->SetupAttachment(SceneRoot);RiverVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);RiverVolume->SetCollisionResponseToAllChannels(ECR_Ignore);RiverVolume->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);RiverVolume->SetHiddenInGame(true);
	UnderwaterPostProcess=CreateDefaultSubobject<UPostProcessComponent>(TEXT("UnderwaterPostProcess"));UnderwaterPostProcess->SetupAttachment(RiverVolume);UnderwaterPostProcess->bUnbound=false;UnderwaterPostProcess->BlendRadius=180.f;UnderwaterPostProcess->BlendWeight=1.f;UnderwaterPostProcess->Settings.bOverride_SceneColorTint=true;UnderwaterPostProcess->Settings.SceneColorTint=FLinearColor(.12f,.42f,.48f);UnderwaterPostProcess->Settings.bOverride_ColorSaturation=true;UnderwaterPostProcess->Settings.ColorSaturation=FVector4(.38f,.62f,.68f,1.f);UnderwaterPostProcess->Settings.bOverride_VignetteIntensity=true;UnderwaterPostProcess->Settings.VignetteIntensity=.48f;UnderwaterPostProcess->Settings.bOverride_BloomIntensity=true;UnderwaterPostProcess->Settings.BloomIntensity=.35f;
	Hills=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Hills"));Hills->SetupAttachment(SceneRoot);Hills->SetMobility(EComponentMobility::Static);Hills->SetCollisionEnabled(ECollisionEnabled::NoCollision);Hills->SetCanEverAffectNavigation(false);
	HillCollision=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HillCollision"));HillCollision->SetupAttachment(SceneRoot);HillCollision->SetMobility(EComponentMobility::Static);HillCollision->SetCollisionProfileName(TEXT("BlockAll"));HillCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);HillCollision->SetCanEverAffectNavigation(true);HillCollision->SetVisibility(false,true);HillCollision->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GroundMesh(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	if(GroundMesh.Succeeded()){Ground->SetStaticMesh(GroundMesh.Object);GroundTiles->SetStaticMesh(GroundMesh.Object);}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GroundMaterial(TEXT("/Game/StarterContent/Materials/M_Ground_Grass.M_Ground_Grass"));
	if(GroundMaterial.Succeeded()){Ground->SetMaterial(0,GroundMaterial.Object);GroundTiles->SetMaterial(0,GroundMaterial.Object);}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RockMesh(TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock"));
	if(RockMesh.Succeeded()){Rocks->SetStaticMesh(RockMesh.Object);Hills->SetStaticMesh(RockMesh.Object);}
	if(GroundMesh.Succeeded()){RockCollision->SetStaticMesh(GroundMesh.Object);HillCollision->SetStaticMesh(GroundMesh.Object);}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BushMesh(TEXT("/Game/StarterContent/Props/SM_Bush.SM_Bush"));
	if(BushMesh.Succeeded()){Bushes->SetStaticMesh(BushMesh.Object);GrassA->SetStaticMesh(BushMesh.Object);GrassB->SetStaticMesh(BushMesh.Object);}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WaterMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));if(WaterMesh.Succeeded()){WaterBodies->SetStaticMesh(WaterMesh.Object);for(UStaticMeshComponent* WaterPart:{RiverSurface,RiverUnderSurface,LakeSurfaceA,LakeSurfaceB,LakeSurfaceC})WaterPart->SetStaticMesh(WaterMesh.Object);}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaterMaterial(TEXT("/Game/StarterContent/Materials/M_Water_Lake.M_Water_Lake"));if(WaterMaterial.Succeeded()){WaterBodies->SetMaterial(0,WaterMaterial.Object);for(UStaticMeshComponent* WaterPart:{RiverSurface,RiverUnderSurface,LakeSurfaceA,LakeSurfaceB,LakeSurfaceC})WaterPart->SetMaterial(0,WaterMaterial.Object);}
}

void AOpenWorldTile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);if(!GetWorld()||!GrassA||!GrassB)return;const float Time=GetWorld()->GetTimeSeconds();APawn* Pawn=UGameplayStatics::GetPlayerPawn(this,0);const FVector PlayerLocal=Pawn?GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation()):FVector(1000000.f);
	auto Animate=[&](UHierarchicalInstancedStaticMeshComponent* Group,float Phase)
	{
		const int32 Count=Group->GetInstanceCount();for(int32 Index=0;Index<Count;++Index){FTransform Transform;if(!Group->GetInstanceTransform(Index,Transform,false))continue;const float Distance=FVector::Dist2D(Transform.GetLocation(),PlayerLocal);const float Interaction=Pawn?FMath::Clamp(1.f-Distance/180.f,0.f,1.f)*FMath::Clamp(Pawn->GetVelocity().Size2D()/350.f,0.f,1.f)*14.f:0.f;const float WindPitch=FMath::Sin(Time*1.65f+Phase+Index*.37f)*2.4f;const float WindRoll=FMath::Sin(Time*1.08f+Phase+Index*.21f)*1.6f;const float Yaw=Transform.Rotator().Yaw;Transform.SetRotation(FRotator(WindPitch+Interaction,Yaw,WindRoll).Quaternion());Group->UpdateInstanceTransform(Index,Transform,false,Index==Count-1,true);}
	};
	Animate(GrassA,0.f);Animate(GrassB,2.2f);
}

void AOpenWorldTile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Ground->SetRelativeLocation(FVector(0.f,0.f,-50.f));
	Ground->SetRelativeScale3D(FVector(TileSize/100.f,TileSize/100.f,1.f));
	GroundCollision->SetRelativeLocation(FVector::ZeroVector);
	GroundCollision->SetBoxExtent(FVector(TileSize*.5f,TileSize*.5f,50.f),false);
	RebuildGroundTiles();
	RebuildWaterBodies();
	RebuildDecorations();
}

void AOpenWorldTile::BeginPlay()
{
	Super::BeginPlay();
	// A streamed instance has its final level transform only at runtime, so rebuild
	// decoration from that position to keep every sector deterministic but different.
	RebuildDecorations();
	RebuildGroundTiles();
	RebuildWaterBodies();
	if(GetNetMode()!=NM_Client)SpawnHarvestableTrees();
	UNavigationSystemV1::UpdateActorInNavOctree(*this);
	if(UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		Navigation->AddDirtyArea(GetComponentsBoundingBox(true),ENavigationDirtyFlag::All);
	}
}

void AOpenWorldTile::RebuildGroundTiles()
{
	if(!GroundTiles||!Ground||!Terrain)return;GroundTiles->ClearInstances();GroundTiles->SetVisibility(false,true);Ground->SetVisibility(false,true);
	// Keep a small independent safety floor only under the deliberately flat spawn
	// clearing. Everywhere else the correctly wound procedural triangles provide
	// the actual sloped collision.
	if(FMath::Abs(GetActorLocation().X)<TileSize*.25f&&FMath::Abs(GetActorLocation().Y)<TileSize*.25f){GroundCollision->SetRelativeLocation(FVector(0.f,0.f,-100.f));GroundCollision->SetBoxExtent(FVector(1800.f,1800.f,100.f),false);GroundCollision->SetCollisionProfileName(TEXT("BlockAll"));GroundCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);}else GroundCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	const int32 Resolution=FMath::Clamp(TerrainResolution,8,128),Side=Resolution+1;const float Step=TileSize/Resolution;TArray<FVector> Vertices;TArray<int32> Triangles;TArray<FVector> Normals;TArray<FVector2D> UVs;TArray<FColor> Colors;TArray<FProcMeshTangent> Tangents;Vertices.Reserve(Side*Side);UVs.Reserve(Side*Side);
	for(int32 Y=0;Y<Side;++Y)for(int32 X=0;X<Side;++X){const float LX=-TileSize*.5f+X*Step,LY=-TileSize*.5f+Y*Step;const float WX=GetActorLocation().X+LX,WY=GetActorLocation().Y+LY;Vertices.Add(FVector(LX,LY,GetTerrainHeight(WX,WY)));UVs.Add(FVector2D(WX/350.f,WY/350.f));}
	// UE4's local vertex factory expects clockwise winding when viewed from the
	// visible side. This order faces the terrain upward instead of rendering its
	// unlit underside and producing one-way collision in the wrong direction.
	for(int32 Y=0;Y<Resolution;++Y)for(int32 X=0;X<Resolution;++X){const int32 A=Y*Side+X,B=A+1,C=A+Side,D=C+1;Triangles.Append({A,C,B,B,C,D});}
	Normals.SetNumZeroed(Vertices.Num());for(int32 I=0;I<Triangles.Num();I+=3){const int32 A=Triangles[I],B=Triangles[I+1],C=Triangles[I+2];const FVector N=-FVector::CrossProduct(Vertices[B]-Vertices[A],Vertices[C]-Vertices[A]).GetSafeNormal();Normals[A]+=N;Normals[B]+=N;Normals[C]+=N;}for(FVector& N:Normals)N.Normalize();Tangents.Init(FProcMeshTangent(1.f,0.f,0.f),Vertices.Num());Colors.Init(FColor::White,Vertices.Num());
	Terrain->ClearAllMeshSections();Terrain->CreateMeshSection(0,Vertices,Triangles,Normals,UVs,Colors,Tangents,true);
	Terrain->SetCollisionProfileName(TEXT("BlockAll"));Terrain->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if(UMaterialInterface* Grass=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/StarterContent/Materials/M_Ground_Grass.M_Ground_Grass")))Terrain->SetMaterial(0,Grass);
}

float AOpenWorldTile::GetTerrainHeight(float X,float Y)const
{
	const float DistanceFromStart=FVector2D(X,Y).Size();const float StartBlend=FMath::SmoothStep(0.f,1.f,FMath::Clamp((DistanceFromStart-1800.f)/3200.f,0.f,1.f));
	const float Broad=FMath::PerlinNoise2D(FVector2D(X,Y)/9000.f)*850.f;const float Medium=FMath::PerlinNoise2D(FVector2D(X+17311.f,Y-8191.f)/3600.f)*260.f;float Height=(Broad+Medium)*StartBlend;
	// Continuous river channel near the starting area. The centre is a deep bed,
	// while the outer 14 metres blend smoothly back into the natural terrain.
	const float RiverCenterY=2800.f+FMath::Sin(X/5200.f)*520.f;const float RiverDistance=FMath::Abs(Y-RiverCenterY);constexpr float WaterHalfWidth=900.f,OuterBankWidth=1500.f;
	if(RiverDistance<=WaterHalfWidth){const float Across=RiverDistance/WaterHalfWidth;Height=FMath::Min(Height,-480.f+450.f*FMath::Square(Across));}
	else if(RiverDistance<OuterBankWidth){const float Bank=(RiverDistance-WaterHalfWidth)/(OuterBankWidth-WaterHalfWidth);Height=FMath::Lerp(-30.f,Height,FMath::SmoothStep(0.f,1.f,Bank));}
	return Height;
}

void AOpenWorldTile::RebuildWaterBodies()
{
	if(!WaterBodies||!RiverSurface||!RiverMesh)return;WaterBodies->ClearInstances();WaterBodies->SetVisibility(false,true);RiverMesh->ClearAllMeshSections();RiverMesh->SetVisibility(false,true);
	UStaticMesh* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));UMaterialInterface* Water=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/StarterContent/Materials/M_Water_Lake.M_Water_Lake"));
	for(UStaticMeshComponent* Part:{RiverSurface,RiverUnderSurface,LakeSurfaceA,LakeSurfaceB,LakeSurfaceC}){if(Plane)Part->SetStaticMesh(Plane);if(Water)Part->SetMaterial(0,Water);Part->SetVisibility(false,true);}
	const FIntPoint Sector(FMath::RoundToInt(GetActorLocation().X/TileSize),FMath::RoundToInt(GetActorLocation().Y/TileSize));FRandomStream Random(HashCombine(GetTypeHash(Sector.X),GetTypeHash(Sector.Y))^0xA73F);
	// The central world row always has a visible river, so water is immediately
	// discoverable. Further rows repeat it sparsely across the streamed world.
	if(Sector.Y==0)
	{
		RiverSurface->SetVisibility(false,true);RiverUnderSurface->SetVisibility(false,true);
		const int32 Segments=96;TArray<FVector> V;TArray<int32> T;TArray<FVector> N;TArray<FVector2D> UV;TArray<FColor> C;TArray<FProcMeshTangent> Tan;V.Reserve((Segments+1)*4);
		// The mesh deliberately continues beneath the rising outer bank. Its actual
		// polygon edge is therefore buried in terrain and cannot expose pinholes
		// caused by the two meshes using different triangle layouts.
		for(int32 I=0;I<=Segments;++I){const float A=static_cast<float>(I)/Segments,LX=-TileSize*.5f-180.f+A*(TileSize+360.f),WX=GetActorLocation().X+LX;const float CY=2800.f+FMath::Sin(WX/5200.f)*520.f-GetActorLocation().Y;constexpr float Width=980.f;V.Add(FVector(LX,CY-Width,-180.f));V.Add(FVector(LX,CY+Width,-180.f));V.Add(FVector(LX,CY-Width,-182.f));V.Add(FVector(LX,CY+Width,-182.f));UV.Append({FVector2D(A*18.f,0.f),FVector2D(A*18.f,1.f),FVector2D(A*18.f,0.f),FVector2D(A*18.f,1.f)});N.Append({FVector::UpVector,FVector::UpVector,-FVector::UpVector,-FVector::UpVector});}
		for(int32 I=0;I<Segments;++I){const int32 A=I*4,B=A+1,D=(I+1)*4,E=D+1;T.Append({A,D,B,B,D,E});const int32 AU=A+2,BU=B+2,DU=D+2,EU=E+2;T.Append({AU,BU,DU,BU,EU,DU});}C.Init(FColor::White,V.Num());Tan.Init(FProcMeshTangent(1.f,0.f,0.f),V.Num());RiverMesh->CreateMeshSection(0,V,T,N,UV,C,Tan,false);if(Water)RiverMesh->SetMaterial(0,Water);RiverMesh->SetVisibility(true,true);
		const float WorldCenterX=GetActorLocation().X,WorldRiverY=2800.f+FMath::Sin(WorldCenterX/5200.f)*520.f,LocalRiverY=WorldRiverY-GetActorLocation().Y;RiverVolume->SetRelativeLocation(FVector(0.f,LocalRiverY,-330.f));RiverVolume->SetRelativeRotation(FRotator::ZeroRotator);RiverVolume->SetBoxExtent(FVector(TileSize*.52f,1050.f,150.f),false);RiverVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);UnderwaterPostProcess->SetVisibility(true,true);
	}
	else{RiverVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);UnderwaterPostProcess->SetVisibility(false,true);}
	// The old test lake planes remain hidden. Their rotated rectangular edges
	// intersected the river and produced detached triangular water fragments.
}

void AOpenWorldTile::SpawnHarvestableTrees()
{
	if(!GetWorld()||TreeCount<=0)return;
	for(AHarvestableTree* Tree:SpawnedTrees)if(IsValid(Tree))Tree->Destroy();SpawnedTrees.Reset();
	const FIntPoint Sector(FMath::RoundToInt(GetActorLocation().X/TileSize),FMath::RoundToInt(GetActorLocation().Y/TileSize));FRandomStream Random(HashCombine(GetTypeHash(Sector.X),GetTypeHash(Sector.Y))^0x7EED);
	for(int32 Index=0;Index<TreeCount;++Index)
	{
		FVector Local(Random.FRandRange(-TileSize*.46f,TileSize*.46f),Random.FRandRange(-TileSize*.46f,TileSize*.46f),0.f);if(Local.SizeSquared2D()<FMath::Square(1500.f))Local.X+=FMath::Sign(Local.X==0.f?1.f:Local.X)*1700.f;Local.Z=GetTerrainHeight(GetActorLocation().X+Local.X,GetActorLocation().Y+Local.Y);
		const float WX=GetActorLocation().X+Local.X,WY=GetActorLocation().Y+Local.Y,RiverY=2800.f+FMath::Sin(WX/5200.f)*520.f;if(FMath::Abs(WY-RiverY)<1750.f)continue;
		FActorSpawnParameters Params;Params.Owner=this;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if(AHarvestableTree* Tree=GetWorld()->SpawnActor<AHarvestableTree>(AHarvestableTree::StaticClass(),GetActorTransform().TransformPosition(Local),FRotator(0.f,Random.FRandRange(0.f,360.f),0.f),Params)){Tree->SetActorScale3D(FVector(Random.FRandRange(.8f,1.25f)));SpawnedTrees.Add(Tree);}
	}
}

void AOpenWorldTile::RebuildDecorations()
{
	if(!Rocks||!RockCollision||!Bushes||!GrassA||!GrassB||!Hills||!HillCollision)return;
	Rocks->ClearInstances();
	RockCollision->ClearInstances();
	Bushes->ClearInstances();GrassA->ClearInstances();GrassB->ClearInstances();
	Hills->ClearInstances();HillCollision->ClearInstances();
	const FIntPoint Sector(FMath::RoundToInt(GetActorLocation().X/TileSize),FMath::RoundToInt(GetActorLocation().Y/TileSize));
	FRandomStream Random(HashCombine(GetTypeHash(Sector.X),GetTypeHash(Sector.Y)));
	for(int32 Index=0;Index<RockCount;++Index)
	{
		FVector Position(Random.FRandRange(-TileSize*.46f,TileSize*.46f),Random.FRandRange(-TileSize*.46f,TileSize*.46f),0.f);
		if(Position.SizeSquared2D()<FMath::Square(1200.f))Position.X+=FMath::Sign(Position.X==0.f?1.f:Position.X)*1400.f;
		const float WX=GetActorLocation().X+Position.X,WY=GetActorLocation().Y+Position.Y,RiverY=2800.f+FMath::Sin(WX/5200.f)*520.f;if(FMath::Abs(WY-RiverY)<1750.f)continue;
		Position.Z=GetTerrainHeight(GetActorLocation().X+Position.X,GetActorLocation().Y+Position.Y);
		const float Scale=Random.FRandRange(.7f,2.4f);
		const FRotator Rotation(0.f,Random.FRandRange(0.f,360.f),0.f);
		Rocks->AddInstance(FTransform(Rotation,Position,FVector(Scale)));
		// SM_Rock has no reliable simple collision in this content pack. A hidden
		// cube approximates the solid core and blocks players and navigation.
		RockCollision->AddInstance(FTransform(Rotation,Position+FVector(0.f,0.f,65.f*Scale),FVector(1.15f*Scale,1.15f*Scale,1.3f*Scale)));
	}
	for(int32 Index=0;Index<BushCount;++Index)
	{
		FVector Position(Random.FRandRange(-TileSize*.47f,TileSize*.47f),Random.FRandRange(-TileSize*.47f,TileSize*.47f),0.f);
		if(Position.SizeSquared2D()<FMath::Square(1100.f))Position.Y+=FMath::Sign(Position.Y==0.f?1.f:Position.Y)*1300.f;
		const float WX=GetActorLocation().X+Position.X,WY=GetActorLocation().Y+Position.Y,RiverY=2800.f+FMath::Sin(WX/5200.f)*520.f;if(FMath::Abs(WY-RiverY)<1600.f)continue;
		Position.Z=GetTerrainHeight(GetActorLocation().X+Position.X,GetActorLocation().Y+Position.Y);
		const float Scale=Random.FRandRange(.65f,1.45f);
		const FRotator Rotation(0.f,Random.FRandRange(0.f,360.f),0.f);
		Bushes->AddInstance(FTransform(Rotation,Position,FVector(Scale)));
	}
	for(int32 Index=0;Index<GrassCount;++Index)
	{
		FVector Position(Random.FRandRange(-TileSize*.48f,TileSize*.48f),Random.FRandRange(-TileSize*.48f,TileSize*.48f),0.f);const float WX=GetActorLocation().X+Position.X,WY=GetActorLocation().Y+Position.Y,RiverY=2800.f+FMath::Sin(WX/5200.f)*520.f;if(FMath::Abs(WY-RiverY)<1650.f)continue;Position.Z=GetTerrainHeight(WX,WY);const float Scale=Random.FRandRange(.12f,.28f);UHierarchicalInstancedStaticMeshComponent* Group=Index%2?GrassA:GrassB;Group->AddInstance(FTransform(FRotator(0.f,Random.FRandRange(0.f,360.f),0.f),Position,FVector(Scale*.55f,Scale,Scale*.45f)));
	}
}
