#include "BuildableStructure.h"

#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"

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
void AWoodGate::OnRep_Open(){if(DoorPieces){DoorPieces->SetRelativeRotation(bOpen?FRotator(0,90,0):FRotator::ZeroRotator);DoorPieces->SetCollisionEnabled(bOpen?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);}}
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
