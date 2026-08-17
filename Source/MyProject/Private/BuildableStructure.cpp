#include "BuildableStructure.h"

#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "OpenWorldStreamingManager.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavLinkComponent.h"

ABuildableStructure::ABuildableStructure()
{
	bReplicates=true;SetReplicateMovement(false);SetCanBeDamaged(true);PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.2f;
	SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));RootComponent=SceneRoot;
}
void ABuildableStructure::BeginPlay(){Super::BeginPlay();StructureHealth=FMath::Min(StructureHealth,MaxStructureHealth);}
void ABuildableStructure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if(!HasAuthority()||bConstructionPreview)return;
	if(bCollapsing)
	{
		CollapseVelocityZ-=980.f*DeltaSeconds;
		const float Fall=CollapseVelocityZ*DeltaSeconds;AddActorWorldOffset(FVector(0.f,0.f,Fall),true);CollapseDistance+=FMath::Abs(Fall);
		if(CollapseDistance>700.f)Destroy();return;
	}
	SupportCheckDelay-=DeltaSeconds;
	if(SupportCheckDelay<=0.f){SupportCheckDelay=.45f;if(!HasStructuralSupport())BeginCollapse();}
}
void ABuildableStructure::BeginCollapse()
{
	if(bCollapsing)return;bCollapsing=true;SetCanBeDamaged(false);SetReplicateMovement(true);CollapseVelocityZ=-80.f;
}
bool ABuildableStructure::HasStructuralSupport()const
{
	if(!bNeedsFoundationSupport||!GetWorld())return true;
	const FVector Here=GetActorLocation();
	FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(StructureFoundationGround),false,this);
	for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)GroundQuery.AddIgnoredActor(*It);
	FCollisionObjectQueryParams GroundObjects;GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	FHitResult Ground;
	if(GetWorld()->LineTraceSingleByObjectType(Ground,Here+FVector(0,0,25.f),Here-FVector(0,0,55.f),GroundObjects,GroundQuery))return true;
	for(TActorIterator<AWoodFloor> It(GetWorld());It;++It)
	{
		if(It->IsCollapsing())continue;
		if(FMath::Abs(Here.Z-It->GetActorLocation().Z)<=24.f&&FVector::Dist2D(Here,It->GetActorLocation())<=230.f)return true;
	}
	// Upper-storey walls around a stair opening may have no floor tile directly
	// beneath them. A matching wall, gate or pillar one storey lower is a valid
	// load-bearing support and should carry the module above it.
	for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)
	{
		if(*It==this||It->IsCollapsing())continue;
		if(!(It->IsA<AWoodWall>()||It->IsA<AWoodGate>()||It->IsA<AWoodPillar>()))continue;
		const FVector Other=It->GetActorLocation();
		if(FMath::Abs(Here.Z-(Other.Z+220.f))<=30.f&&FVector::Dist2D(Here,Other)<=85.f)return true;
	}
	return false;
}
void ABuildableStructure::GetSnapPoints(TArray<FVector>& OutPoints)const
{
	const FVector Along=GetActorRotation().RotateVector(FVector::RightVector);
	OutPoints.Add(GetActorLocation()+Along*HalfModuleLength);OutPoints.Add(GetActorLocation()-Along*HalfModuleLength);
}
float ABuildableStructure::TakeDamage(float Amount,const FDamageEvent&,AController*,AActor*)
{
	if(!HasAuthority()||Amount<=0.f)return 0.f;
	const float Applied=FMath::Min(StructureHealth,Amount);StructureHealth-=Applied;if(StructureHealth<=0.f)Destroy();return Applied;
}
void ABuildableStructure::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ABuildableStructure,StructureHealth);}

AWoodWall::AWoodWall()
{
	MaxStructureHealth=350.f;StructureHealth=350.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Pieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallPieces"));Pieces->SetupAttachment(SceneRoot);
	if(CubeAsset.Succeeded())Pieces->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Pieces->SetMaterial(0,WoodAsset.Object);Pieces->SetCollisionProfileName(TEXT("BlockAll"));
	for(int32 Row=0;Row<5;++Row)Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,0,22.f+Row*42.f),FVector(.22f,3.f,.18f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,-145,105),FVector(.3f,.22f,2.15f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,145,105),FVector(.3f,.22f,2.15f)));
}

AWoodGate::AWoodGate()
{
	MaxStructureHealth=500.f;StructureHealth=500.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Pieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GatePieces"));Pieces->SetupAttachment(SceneRoot);
	if(CubeAsset.Succeeded())Pieces->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Pieces->SetMaterial(0,WoodAsset.Object);Pieces->SetCollisionProfileName(TEXT("BlockAll"));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,-145,110),FVector(.34f,.28f,2.25f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,145,110),FVector(.34f,.28f,2.25f)));
	DoorPieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GateDoors"));DoorPieces->SetupAttachment(SceneRoot);
	if(CubeAsset.Succeeded())DoorPieces->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())DoorPieces->SetMaterial(0,WoodAsset.Object);DoorPieces->SetCollisionProfileName(TEXT("BlockAll"));
	for(int32 Index=0;Index<4;++Index)DoorPieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,-90.f+Index*60.f,105),FVector(.20f,.48f,1.85f)));
	DoorPieces->AddInstance(FTransform(FRotator(32.f,0,0),FVector(0,0,105),FVector(.22f,2.75f,.13f)));
	DoorPieces->AddInstance(FTransform(FRotator(-32.f,0,0),FVector(0,0,105),FVector(.22f,2.75f,.13f)));
}
bool AWoodGate::TryToggle(AActor* User){if(!HasAuthority()||!User||FVector::DistSquared(User->GetActorLocation(),GetActorLocation())>FMath::Square(350.f))return false;bOpen=!bOpen;OnRep_Open();return true;}
void AWoodGate::OnRep_Open(){if(DoorPieces){DoorPieces->SetRelativeRotation(bOpen?FRotator(0,90,0):FRotator::ZeroRotator);DoorPieces->SetCollisionEnabled(bOpen?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);UNavigationSystemV1::UpdateComponentInNavOctree(*DoorPieces);}}
void AWoodGate::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWoodGate,bOpen);}

AWoodFloor::AWoodFloor()
{
	MaxStructureHealth=300.f;StructureHealth=300.f;HalfModuleLength=150.f;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Pieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorPieces"));Pieces->SetupAttachment(SceneRoot);
	if(CubeAsset.Succeeded())Pieces->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Pieces->SetMaterial(0,WoodAsset.Object);Pieces->SetCollisionProfileName(TEXT("BlockAll"));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,0.f,6.f),FVector(3.f,3.f,.12f)));
}
void AWoodFloor::GetSnapPoints(TArray<FVector>& OutPoints)const
{
	const FVector Right=GetActorRotation().RotateVector(FVector::RightVector).GetSafeNormal2D();
	const FVector Forward=GetActorRotation().RotateVector(FVector::ForwardVector).GetSafeNormal2D();
	OutPoints.Add(GetActorLocation()+Right*150.f);OutPoints.Add(GetActorLocation()-Right*150.f);
	OutPoints.Add(GetActorLocation()+Forward*150.f);OutPoints.Add(GetActorLocation()-Forward*150.f);
}
bool AWoodFloor::HasStructuralSupport()const
{
	if(!GetWorld())return true;
	auto HasDirectSupport=[this](const AWoodFloor* Floor)
	{
		const FVector Here=Floor->GetActorLocation();
		FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(FloorSupportGround),false,Floor);
		for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)GroundQuery.AddIgnoredActor(*It);
		FCollisionObjectQueryParams GroundObjects;GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);FHitResult Ground;
		if(GetWorld()->LineTraceSingleByObjectType(Ground,Here+FVector(0,0,20.f),Here-FVector(0,0,45.f),GroundObjects,GroundQuery))return true;
		for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)
		{
			if(*It==Floor||It->IsPendingKill()||It->IsCollapsing())continue;
			const FVector Other=It->GetActorLocation();const float XY=FVector::Dist2D(Here,Other);
			if((It->IsA<AWoodWall>()||It->IsA<AWoodGate>())&&FMath::Abs(Here.Z-(Other.Z+220.f))<=28.f&&XY<=235.f)return true;
			if(It->IsA<AWoodStairs>()&&FMath::Abs(Here.Z-(Other.Z+220.f))<=28.f&&XY<=340.f)return true;
			if(It->IsA<AWoodPillar>()&&FMath::Abs(Here.Z-(Other.Z+220.f))<=28.f&&XY<=185.f)return true;
		}
		return false;
	};

	// A directly supported tile is the anchor. Up to three adjacent 3 m floor
	// modules may cantilever from that anchor; the fourth has no valid support
	// path and will enter the normal replicated collapse simulation.
	TArray<const AWoodFloor*> Frontier;Frontier.Add(this);TSet<const AWoodFloor*> Visited;Visited.Add(this);
	for(int32 Depth=0;Depth<=3;++Depth)
	{
		for(const AWoodFloor* Floor:Frontier)if(HasDirectSupport(Floor))return true;
		if(Depth==3)break;
		TArray<const AWoodFloor*> Next;
		for(const AWoodFloor* Floor:Frontier)for(TActorIterator<AWoodFloor> It(GetWorld());It;++It)
		{
			const AWoodFloor* Neighbor=*It;if(!Neighbor||Neighbor->IsCollapsing()||Visited.Contains(Neighbor))continue;
			const float XY=FVector::Dist2D(Floor->GetActorLocation(),Neighbor->GetActorLocation());
			if(FMath::Abs(Floor->GetActorLocation().Z-Neighbor->GetActorLocation().Z)<=28.f&&XY>=260.f&&XY<=340.f){Visited.Add(Neighbor);Next.Add(Neighbor);}
		}
		Frontier=MoveTemp(Next);if(Frontier.Num()==0)break;
	}
	return false;
}

AWoodStairs::AWoodStairs()
{
	MaxStructureHealth=400.f;StructureHealth=400.f;HalfModuleLength=150.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Pieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StairPieces"));Pieces->SetupAttachment(SceneRoot);
	if(CubeAsset.Succeeded())Pieces->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Pieces->SetMaterial(0,WoodAsset.Object);Pieces->SetCollisionProfileName(TEXT("BlockAll"));
	for(int32 Step=0;Step<10;++Step)Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,-135.f+Step*30.f,11.f+Step*22.f),FVector(1.8f,.3f,.22f)));
	// A physical hidden ramp blocks the player's capsule.  A navigation link gives
	// Recast the same bottom-to-top connection without adding collision to stairs.
	NavigationLink=CreateDefaultSubobject<UNavLinkComponent>(TEXT("StairNavigationLink"));NavigationLink->SetupAttachment(SceneRoot);
	FNavigationLink StairLink(FVector(0.f,-150.f,15.f),FVector(0.f,150.f,220.f));
	StairLink.Direction=ENavLinkDirection::BothWays;StairLink.SnapRadius=120.f;StairLink.bUseSnapHeight=true;StairLink.SnapHeight=180.f;
	NavigationLink->Links.Add(StairLink);
}
void AWoodStairs::GetSnapPoints(TArray<FVector>& OutPoints)const
{
	const FVector Along=GetActorRotation().RotateVector(FVector::RightVector).GetSafeNormal2D();
	OutPoints.Add(GetActorLocation()-Along*150.f);
	OutPoints.Add(GetActorLocation()+Along*150.f+FVector(0.f,0.f,220.f));
}

AWoodPillar::AWoodPillar()
{
	MaxStructureHealth=300.f;StructureHealth=300.f;HalfModuleLength=0.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Piece=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarPiece"));Piece->SetupAttachment(SceneRoot);
	if(CubeAsset.Succeeded())Piece->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Piece->SetMaterial(0,WoodAsset.Object);
	Piece->SetCollisionProfileName(TEXT("BlockAll"));Piece->SetRelativeLocation(FVector(0.f,0.f,110.f));Piece->SetRelativeScale3D(FVector(.28f,.28f,2.2f));
}

AWallTorch::AWallTorch()
{
	MaxStructureHealth=80.f;StructureHealth=80.f;HalfModuleLength=0.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Pole=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TorchPole"));Pole->SetupAttachment(SceneRoot);
	if(Cylinder.Succeeded())Pole->SetStaticMesh(Cylinder.Object);if(BasicMaterial.Succeeded())Pole->SetMaterial(0,BasicMaterial.Object);
	Pole->SetRelativeLocation(FVector(0.f,0.f,-24.f));Pole->SetRelativeScale3D(FVector(.055f,.055f,.72f));Pole->SetCollisionProfileName(TEXT("BlockAll"));
	Bracket=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallBracket"));Bracket->SetupAttachment(SceneRoot);
	if(Cube.Succeeded())Bracket->SetStaticMesh(Cube.Object);if(BasicMaterial.Succeeded())Bracket->SetMaterial(0,BasicMaterial.Object);
	Bracket->SetRelativeLocation(FVector(-18.f,0.f,-45.f));Bracket->SetRelativeScale3D(FVector(.36f,.07f,.07f));Bracket->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flame=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flame"));Flame->SetupAttachment(SceneRoot);
	if(Sphere.Succeeded())Flame->SetStaticMesh(Sphere.Object);if(BasicMaterial.Succeeded())Flame->SetMaterial(0,BasicMaterial.Object);
	Flame->SetRelativeLocation(FVector(0.f,0.f,52.f));Flame->SetRelativeScale3D(FVector(.12f,.12f,.2f));Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TorchLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("TorchLight"));TorchLight->SetupAttachment(SceneRoot);
	TorchLight->SetRelativeLocation(FVector(12.f,0.f,58.f));TorchLight->SetLightColor(FLinearColor(1.f,.32f,.055f));TorchLight->SetIntensity(5600.f);TorchLight->SetAttenuationRadius(1250.f);TorchLight->SetCastShadows(false);Flame->SetCastShadow(false);
}

void AWallTorch::BeginPlay()
{
	Super::BeginPlay();
	auto Tint=[](UStaticMeshComponent* Component,const FLinearColor& Color){if(UMaterialInstanceDynamic* Material=Component?Component->CreateAndSetMaterialInstanceDynamic(0):nullptr)Material->SetVectorParameterValue(TEXT("Color"),Color);};
	Tint(Pole,FLinearColor(.12f,.035f,.008f));Tint(Bracket,FLinearColor(.08f,.025f,.006f));Tint(Flame,FLinearColor(1.f,.08f,.005f));
	UpdateTorchLight();
}

void AWallTorch::Tick(float DeltaSeconds){Super::Tick(DeltaSeconds);UpdateTorchLight();}

void AWallTorch::UpdateTorchLight()
{
	const bool bLit=!bConstructionPreview&&!bCollapsing;
	if(TorchLight){TorchLight->SetVisibility(bLit,true);if(bLit&&GetWorld())TorchLight->SetIntensity(5300.f+FMath::Sin(GetWorld()->GetTimeSeconds()*8.7f)*300.f);}
	if(Flame)Flame->SetVisibility(bLit,true);
}

bool AWallTorch::HasStructuralSupport()const
{
	if(!GetWorld())return false;
	for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)
	{
		if(It->IsConstructionPreview()||It->IsCollapsing()||!(It->IsA<AWoodWall>()||It->IsA<AWoodGate>()))continue;
		const FVector Local=It->GetActorTransform().InverseTransformPosition(GetActorLocation());
		if(FMath::Abs(Local.X)<=75.f&&FMath::Abs(Local.Y)<=175.f&&Local.Z>=-20.f&&Local.Z<=250.f)return true;
	}
	return false;
}
