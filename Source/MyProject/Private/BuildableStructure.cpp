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
#include "WoodAxeWeapon.h"
#include "ShooterCharacter.h"
#include "InventoryComponent.h"

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
		if(!(It->IsA<AWoodWall>()||It->IsA<AWoodWindowWall>()||It->IsA<AWoodGate>()||It->IsA<AWoodPillar>()))continue;
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
float ABuildableStructure::TakeDamage(float Amount,const FDamageEvent&,AController* EventInstigator,AActor* DamageCauser)
{
	if(!HasAuthority()||Amount<=0.f)return 0.f;
	// Every wooden structure takes exactly ten axe strikes from full health.
	// Other damage sources (zombies, bullets, explosions) retain normal damage.
	if(Cast<AWoodAxeWeapon>(DamageCauser))Amount=MaxStructureHealth/FMath::Max(1,AxeHitsToDestroy);
	const float Applied=FMath::Min(StructureHealth,Amount);StructureHealth-=Applied;
	if(StructureHealth<=0.f)
	{
		if(AWoodAxeWeapon* Axe=Cast<AWoodAxeWeapon>(DamageCauser))
		{
			AShooterCharacter* Gatherer=Cast<AShooterCharacter>(Axe->GetOwner());
			if(!Gatherer&&EventInstigator)Gatherer=Cast<AShooterCharacter>(EventInstigator->GetPawn());
			if(Gatherer&&Gatherer->Inventory)
			{
				int32 WoodReward=2;
				if(IsA<AWoodWall>()||IsA<AWoodWindowWall>())WoodReward=4;
				else if(IsA<AWoodGate>()||IsA<AWoodDoor>())WoodReward=6;
				const int32 Added=Gatherer->Inventory->AddItemPartial(TEXT("Wood"),WoodReward);
				if(Added>0)Gatherer->ShowLocalNotification(FString::Printf(TEXT("ДЕРЕВО +%d"),Added),2.f);
			}
		}
		Destroy();
	}
	return Applied;
}
void ABuildableStructure::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ABuildableStructure,StructureHealth);}

AWoodWall::AWoodWall()
{
	MaxStructureHealth=350.f;StructureHealth=350.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Pieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallPieces"));Pieces->SetupAttachment(SceneRoot);
	if(CubeAsset.Succeeded())Pieces->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Pieces->SetMaterial(0,WoodAsset.Object);Pieces->SetCollisionProfileName(TEXT("BlockAll"));
	// Ten slightly overlapping planks fill the complete 3 x 2.1 m module. The
	// former five boards left sight/fire gaps large enough to see through.
	for(int32 Row=0;Row<10;++Row)Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,0,10.5f+Row*21.f),FVector(.22f,3.f,.215f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,-145,105),FVector(.3f,.22f,2.15f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,145,105),FVector(.3f,.22f,2.15f)));
}

AWoodWindowWall::AWoodWindowWall()
{
	MaxStructureHealth=325.f;StructureHealth=325.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Pieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WindowWallPieces"));Pieces->SetupAttachment(SceneRoot);
	if(CubeAsset.Succeeded())Pieces->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Pieces->SetMaterial(0,WoodAsset.Object);Pieces->SetCollisionProfileName(TEXT("BlockAll"));
	// Solid lower and upper bands plus side jambs leave one deliberate opening
	// (180 x 80 cm) while the rest of the module remains opaque and blocking.
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,0,35.f),FVector(.22f,3.f,.70f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,0,180.f),FVector(.22f,3.f,.60f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,-120.f,110.f),FVector(.22f,.60f,.80f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,120.f,110.f),FVector(.22f,.60f,.80f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,-145.f,105.f),FVector(.30f,.22f,2.15f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,145.f,105.f),FVector(.30f,.22f,2.15f)));
}

AWoodGate::AWoodGate()
{
	MaxStructureHealth=500.f;StructureHealth=500.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Pieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GatePieces"));Pieces->SetupAttachment(SceneRoot);
	Pieces->SetRelativeLocation(FVector(0.f,0.f,-30.f));
	if(CubeAsset.Succeeded())Pieces->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Pieces->SetMaterial(0,WoodAsset.Object);Pieces->SetCollisionProfileName(TEXT("BlockAll"));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,-145,100),FVector(.34f,.28f,2.30f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,145,100),FVector(.34f,.28f,2.30f)));
	LeftHinge=CreateDefaultSubobject<USceneComponent>(TEXT("LeftGateHinge"));LeftHinge->SetupAttachment(SceneRoot);LeftHinge->SetRelativeLocation(FVector(0.f,-145.f,-30.f));
	RightHinge=CreateDefaultSubobject<USceneComponent>(TEXT("RightGateHinge"));RightHinge->SetupAttachment(SceneRoot);RightHinge->SetRelativeLocation(FVector(0.f,145.f,-30.f));
	DoorPieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LeftGateDoor"));DoorPieces->SetupAttachment(LeftHinge);
	RightDoorPieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RightGateDoor"));RightDoorPieces->SetupAttachment(RightHinge);
	for(UInstancedStaticMeshComponent* Door:{DoorPieces,RightDoorPieces})
	{
		if(CubeAsset.Succeeded())Door->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Door->SetMaterial(0,WoodAsset.Object);Door->SetCollisionProfileName(TEXT("BlockAll"));
	}
	// Two leaves pivot at the side posts.  Their lower edge is exactly at Z=0,
	// matching the wall/gate module foundation instead of floating above it.
	for(int32 Index=0;Index<2;++Index)
	{
		DoorPieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,36.25f+Index*72.5f,100.f),FVector(.20f,.725f,2.30f)));
		RightDoorPieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,-36.25f-Index*72.5f,100.f),FVector(.20f,.725f,2.30f)));
	}
	DoorPieces->AddInstance(FTransform(FRotator(28.f,0,0),FVector(0.f,72.f,100.f),FVector(.22f,1.35f,.13f)));
	RightDoorPieces->AddInstance(FTransform(FRotator(-28.f,0,0),FVector(0.f,-72.f,100.f),FVector(.22f,1.35f,.13f)));
}
bool AWoodGate::TryToggle(AActor* User){if(!HasAuthority()||!User||FVector::DistSquared(User->GetActorLocation(),GetActorLocation())>FMath::Square(500.f))return false;bOpen=!bOpen;OnRep_Open();return true;}
void AWoodGate::SetOpenForLoad(bool bNewOpen){if(!HasAuthority())return;bOpen=bNewOpen;OnRep_Open();}
void AWoodGate::OnRep_Open()
{
	if(LeftHinge)LeftHinge->SetRelativeRotation(bOpen?FRotator(0,90,0):FRotator::ZeroRotator);
	if(RightHinge)RightHinge->SetRelativeRotation(bOpen?FRotator(0,-90,0):FRotator::ZeroRotator);
	for(UInstancedStaticMeshComponent* Door:{DoorPieces,RightDoorPieces})if(Door)
	{
		Door->SetCollisionEnabled(bOpen?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);
		UNavigationSystemV1::UpdateComponentInNavOctree(*Door);
	}
}
void AWoodGate::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWoodGate,bOpen);}

AWoodDoor::AWoodDoor()
{
	MaxStructureHealth=360.f;StructureHealth=360.f;
	// Re-author the inherited gate components as a conventional single door.
	// Keeping AWoodGate as the base means interaction, replication, saving and AI
	// traversal all work identically without a second incompatible door protocol.
	Pieces->ClearInstances();DoorPieces->ClearInstances();RightDoorPieces->ClearInstances();
	Pieces->SetRelativeLocation(FVector::ZeroVector);
	LeftHinge->SetRelativeLocation(FVector(0.f,-112.f,0.f));
	RightHinge->SetRelativeLocation(FVector::ZeroVector);
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,-140.f,105.f),FVector(.28f,.28f,2.15f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,140.f,105.f),FVector(.28f,.28f,2.15f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,0.f,210.f),FVector(.28f,3.f,.22f)));
	// The leaf pivots at its left jamb and closes the 224 cm clear opening. Its
	// bottom is at the module foundation so there is no crawl-sized gap.
	DoorPieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,112.f,100.f),FVector(.20f,2.24f,2.f)));
	DoorPieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(-4.f,112.f,102.f),FVector(.10f,2.f,.08f)));
	RightDoorPieces->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightDoorPieces->SetVisibility(false,true);
}

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
			if((It->IsA<AWoodWall>()||It->IsA<AWoodWindowWall>()||It->IsA<AWoodGate>())&&FMath::Abs(Here.Z-(Other.Z+220.f))<=28.f&&XY<=235.f)return true;
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
	// Use most of the three-metre module width. The former 180 cm staircase was
	// technically walkable, but too narrow for several character capsules to
	// enter reliably from different approach angles.
	for(int32 Step=0;Step<10;++Step)Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,-135.f+Step*30.f,11.f+Step*22.f),FVector(2.6f,.3f,.22f)));
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
		if(It->IsConstructionPreview()||It->IsCollapsing()||!(It->IsA<AWoodWall>()||It->IsA<AWoodWindowWall>()||It->IsA<AWoodGate>()))continue;
		const FVector Local=It->GetActorTransform().InverseTransformPosition(GetActorLocation());
		if(FMath::Abs(Local.X)<=75.f&&FMath::Abs(Local.Y)<=175.f&&Local.Z>=-20.f&&Local.Z<=250.f)return true;
	}
	return false;
}
