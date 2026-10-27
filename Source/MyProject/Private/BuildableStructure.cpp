#include "BuildableStructure.h"

#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
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

namespace
{
	void ConfigureBuildVisual(UStaticMeshComponent* Component,UStaticMesh* Asset,const FVector& TargetSize,const FVector& TargetCenter)
	{
		if(!Component||!Asset)return;
		Component->SetStaticMesh(Asset);
		const FBoxSphereBounds Bounds=Asset->GetBounds();
		const FVector SourceSize=Bounds.BoxExtent*2.f;
		const FVector Scale(
			TargetSize.X/FMath::Max(SourceSize.X,KINDA_SMALL_NUMBER),
			TargetSize.Y/FMath::Max(SourceSize.Y,KINDA_SMALL_NUMBER),
			TargetSize.Z/FMath::Max(SourceSize.Z,KINDA_SMALL_NUMBER));
		Component->SetRelativeScale3D(Scale);
		Component->SetRelativeLocation(TargetCenter-Bounds.Origin*Scale);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	UBoxComponent* CreateBuildCollision(AActor* Owner,USceneComponent* Parent,const FName Name,const FVector& Center,const FVector& Extent)
	{
		UBoxComponent* Box=Owner->CreateDefaultSubobject<UBoxComponent>(Name);
		Box->SetupAttachment(Parent);Box->SetRelativeLocation(Center);Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));return Box;
	}
}

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
	if(bCollapsing)return;bCollapsing=true;SetActorTickEnabled(true);SetCanBeDamaged(false);SetReplicateMovement(true);CollapseVelocityZ=-80.f;
}
bool ABuildableStructure::HasStructuralSupport()const
{
	if(!bNeedsFoundationSupport||!GetWorld())return true;
	// Procedural houses are validated as a complete structure by their generator.
	// Individual partition modules can sit on the seam between floor tiles and
	// must never fall away later, leaving apparently random holes in rooms.
	if(ActorHasTag(TEXT("GeneratedBuilding")))return true;
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
		if(FMath::Abs(Here.Z-(Other.Z+300.f))<=30.f&&FVector::Dist2D(Here,Other)<=85.f)return true;
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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WallAsset(TEXT("/Game/ThirdPersonBP/Build/Meshes/Wood_wall/wood_wall.wood_wall"));
	Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallVisual"));Visual->SetupAttachment(SceneRoot);
	if(WallAsset.Succeeded())ConfigureBuildVisual(Visual,WallAsset.Object,FVector(24.f,300.f,300.f),FVector(0.f,0.f,150.f));
	Collision=CreateBuildCollision(this,SceneRoot,TEXT("WallCollision"),FVector(0.f,0.f,150.f),FVector(12.f,150.f,150.f));
}

AWoodWindowWall::AWoodWindowWall()
{
	MaxStructureHealth=325.f;StructureHealth=325.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WindowAsset(TEXT("/Game/ThirdPersonBP/Build/Meshes/Wall_with_window_01/wall_with_window_00.wall_with_window_00"));
	Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WindowWallVisual"));Visual->SetupAttachment(SceneRoot);
	if(WindowAsset.Succeeded())
	{
		// The new mesh was authored lengthwise on X and is thin on Y. Scale it in
		// its source axes first, then rotate it into the build system's convention:
		// wall thickness on X, module width on Y.
		ConfigureBuildVisual(Visual,WindowAsset.Object,FVector(300.f,24.f,300.f),FVector(0.f,0.f,150.f));
		Visual->SetRelativeRotation(FRotator(0.f,90.f,0.f));
	}
	// Four boxes follow the solid bands and jambs. The only non-blocking area is
	// the authored 180 x 100 cm window opening.
	CollisionPieces.Add(CreateBuildCollision(this,SceneRoot,TEXT("WindowLowerCollision"),FVector(0.f,0.f,50.f),FVector(12.f,150.f,50.f)));
	CollisionPieces.Add(CreateBuildCollision(this,SceneRoot,TEXT("WindowUpperCollision"),FVector(0.f,0.f,250.f),FVector(12.f,150.f,50.f)));
	CollisionPieces.Add(CreateBuildCollision(this,SceneRoot,TEXT("WindowLeftCollision"),FVector(0.f,-120.f,150.f),FVector(12.f,30.f,50.f)));
	CollisionPieces.Add(CreateBuildCollision(this,SceneRoot,TEXT("WindowRightCollision"),FVector(0.f,120.f,150.f),FVector(12.f,30.f,50.f)));
}

AWoodGate::AWoodGate()
{
	MaxStructureHealth=500.f;StructureHealth=500.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Pieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GatePieces"));Pieces->SetupAttachment(SceneRoot);
	Pieces->SetRelativeLocation(FVector(0.f,0.f,-30.f));
	if(CubeAsset.Succeeded())Pieces->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Pieces->SetMaterial(0,WoodAsset.Object);Pieces->SetCollisionProfileName(TEXT("BlockAll"));
	Pieces->SetRelativeLocation(FVector::ZeroVector);
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,-145,150),FVector(.34f,.28f,3.f)));
	Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0,145,150),FVector(.34f,.28f,3.f)));
	LeftHinge=CreateDefaultSubobject<USceneComponent>(TEXT("LeftGateHinge"));LeftHinge->SetupAttachment(SceneRoot);LeftHinge->SetRelativeLocation(FVector(0.f,-145.f,0.f));
	RightHinge=CreateDefaultSubobject<USceneComponent>(TEXT("RightGateHinge"));RightHinge->SetupAttachment(SceneRoot);RightHinge->SetRelativeLocation(FVector(0.f,145.f,0.f));
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
		DoorPieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,36.25f+Index*72.5f,150.f),FVector(.20f,.725f,3.f)));
		RightDoorPieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,-36.25f-Index*72.5f,150.f),FVector(.20f,.725f,3.f)));
	}
	DoorPieces->AddInstance(FTransform(FRotator(28.f,0,0),FVector(0.f,72.f,100.f),FVector(.22f,1.35f,.13f)));
	RightDoorPieces->AddInstance(FTransform(FRotator(-28.f,0,0),FVector(0.f,-72.f,100.f),FVector(.22f,1.35f,.13f)));
}
bool AWoodGate::TryToggle(AActor* User){if(!HasAuthority()||!User||FVector::DistSquared(User->GetActorLocation(),GetActorLocation())>FMath::Square(500.f))return false;bOpen=!bOpen;OnRep_Open();return true;}
void AWoodGate::SetOpenForLoad(bool bNewOpen){if(!HasAuthority())return;bOpen=bNewOpen;OnRep_Open();}
void AWoodGate::OnRep_Open()
{
	ApplyOpenState();
}
void AWoodGate::ApplyOpenState()
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
	MaxStructureHealth=360.f;StructureHealth=360.f;HalfModuleLength=75.f;
	// Disable the legacy cube-built gate. The imported door is one half-module
	// (150 cm); two independent actors therefore close one ordinary 300 cm bay.
	Pieces->ClearInstances();DoorPieces->ClearInstances();RightDoorPieces->ClearInstances();
	for(UInstancedStaticMeshComponent* Legacy:{Pieces,DoorPieces,RightDoorPieces}){Legacy->SetCollisionEnabled(ECollisionEnabled::NoCollision);Legacy->SetVisibility(false,true);}
	LeftHinge->SetRelativeLocation(FVector(0.f,-75.f,0.f));RightHinge->SetRelativeLocation(FVector::ZeroVector);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DoorAsset(TEXT("/Game/ThirdPersonBP/Build/Meshes/Door_01/door_01.door_01"));
	DoorVisual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorVisual"));DoorVisual->SetupAttachment(LeftHinge);
	// Preserve the material imported with door_01 instead of replacing it with
	// the wall material. This mesh is authored in the X/Z plane (X is width,
	// Y is thickness), while a built door occupies the Y/Z plane. Rotate it
	// around Z first and fit each source axis to the matching doorway axis.
	if(DoorAsset.Succeeded())
	{
		UStaticMesh* Mesh=DoorAsset.Object;
		DoorVisual->SetStaticMesh(Mesh);
		const FBoxSphereBounds Bounds=Mesh->GetBounds();
		const FVector SourceSize=Bounds.BoxExtent*2.f;
		const FVector DoorScale(
			150.f/FMath::Max(SourceSize.X,KINDA_SMALL_NUMBER),
			18.f/FMath::Max(SourceSize.Y,KINDA_SMALL_NUMBER),
			300.f/FMath::Max(SourceSize.Z,KINDA_SMALL_NUMBER));
		const FRotator DoorRotation(0.f,90.f,0.f);
		DoorVisual->SetRelativeScale3D(DoorScale);
		DoorVisual->SetRelativeRotation(DoorRotation);
		DoorVisual->SetRelativeLocation(FVector(0.f,75.f,150.f)-DoorRotation.RotateVector(Bounds.Origin*DoorScale));
		DoorVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	DoorCollision=CreateBuildCollision(this,LeftHinge,TEXT("DoorCollision"),FVector(0.f,75.f,150.f),FVector(9.f,75.f,150.f));
}
void AWoodDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWoodDoor,OpenAngle);}
void AWoodDoor::GetSnapPoints(TArray<FVector>& OutPoints)const
{
	const FVector Along=GetActorRotation().RotateVector(FVector::RightVector);
	OutPoints.Add(GetActorLocation()+Along*75.f);OutPoints.Add(GetActorLocation()-Along*75.f);
}
void AWoodDoor::ApplyOpenState()
{
	if(LeftHinge)LeftHinge->SetRelativeRotation(bOpen?FRotator(0.f,OpenAngle,0.f):FRotator::ZeroRotator);
	if(DoorCollision){DoorCollision->SetCollisionEnabled(bOpen?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);UNavigationSystemV1::UpdateComponentInNavOctree(*DoorCollision);}
}

AWoodFloor::AWoodFloor()
{
	MaxStructureHealth=300.f;StructureHealth=300.f;HalfModuleLength=150.f;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FloorAsset(TEXT("/Game/ThirdPersonBP/Build/Meshes/Wood_floor/wood_floor.wood_floor"));
	Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorVisual"));Visual->SetupAttachment(SceneRoot);
	if(FloorAsset.Succeeded())ConfigureBuildVisual(Visual,FloorAsset.Object,FVector(300.f,300.f,12.f),FVector(0.f,0.f,6.f));
	Collision=CreateBuildCollision(this,SceneRoot,TEXT("FloorCollision"),FVector(0.f,0.f,6.f),FVector(150.f,150.f,6.f));
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
			if((It->IsA<AWoodWall>()||It->IsA<AWoodWindowWall>()||It->IsA<AWoodGate>())&&FMath::Abs(Here.Z-(Other.Z+300.f))<=28.f&&XY<=235.f)return true;
			if(It->IsA<AWoodStairs>()&&FMath::Abs(Here.Z-(Other.Z+300.f))<=28.f&&XY<=340.f)return true;
			if(It->IsA<AWoodPillar>()&&FMath::Abs(Here.Z-(Other.Z+300.f))<=28.f&&XY<=185.f)return true;
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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StairAsset(TEXT("/Game/ThirdPersonBP/Build/Meshes/Wood_stairs/wood_stairs-2.wood_stairs-2"));
	Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StairVisual"));Visual->SetupAttachment(SceneRoot);
	// Slightly overlap both landings. Imported handrails extend farther than the
	// treads, so fitting the complete bounds to exactly 300 cm left the final
	// tread visibly short of the upper floor.
	if(StairAsset.Succeeded())ConfigureBuildVisual(Visual,StairAsset.Object,FVector(300.f,318.f,320.f),FVector(0.f,0.f,160.f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Pieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StairPieces"));Pieces->SetupAttachment(SceneRoot);
	if(CubeAsset.Succeeded())Pieces->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Pieces->SetMaterial(0,WoodAsset.Object);Pieces->SetCollisionProfileName(TEXT("BlockAll"));
	Pieces->SetVisibility(false,true);
	// These boxes used to be the visible and physical steps. Once the continuous
	// ramp was added they protruded through it and the capsule still hit every
	// vertical riser. Keep the instances only as an editor reference, never as
	// collision.
	Pieces->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Use most of the three-metre module width. The former 180 cm staircase was
	// technically walkable, but too narrow for several character capsules to
	// enter reliably from different approach angles.
	for(int32 Step=0;Step<12;++Step)Pieces->AddInstance(FTransform(FRotator::ZeroRotator,FVector(0.f,-137.5f+Step*25.f,12.5f+Step*25.f),FVector(3.f,.255f,.25f)));
	// CharacterMovement walks on one continuous surface instead of colliding with
	// twelve sharp imported risers. A 44-degree incline remains below the default
	// walkable-floor limit and overlaps both landings by roughly six centimetres.
	WalkableRamp=CreateBuildCollision(this,SceneRoot,TEXT("StairWalkableRamp"),FVector(0.f,0.f,150.f),FVector(138.f,216.f,8.f));
	WalkableRamp->SetRelativeRotation(FRotator(0.f,0.f,44.f));
	WalkableRamp->SetHiddenInGame(true);
	// A physical hidden ramp blocks the player's capsule.  A navigation link gives
	// Recast the same bottom-to-top connection without adding collision to stairs.
	NavigationLink=CreateDefaultSubobject<UNavLinkComponent>(TEXT("StairNavigationLink"));NavigationLink->SetupAttachment(SceneRoot);
	FNavigationLink StairLink(FVector(0.f,-150.f,15.f),FVector(0.f,150.f,300.f));
	StairLink.Direction=ENavLinkDirection::BothWays;StairLink.SnapRadius=120.f;StairLink.bUseSnapHeight=true;StairLink.SnapHeight=180.f;
	NavigationLink->Links.Add(StairLink);
}
void AWoodStairs::GetSnapPoints(TArray<FVector>& OutPoints)const
{
	const FVector Along=GetActorRotation().RotateVector(FVector::RightVector).GetSafeNormal2D();
	OutPoints.Add(GetActorLocation()-Along*150.f);
	OutPoints.Add(GetActorLocation()+Along*150.f+FVector(0.f,0.f,300.f));
}
bool AWoodStairs::HasStructuralSupport()const
{
	if(ABuildableStructure::HasStructuralSupport())return true;
	if(!GetWorld())return true;
	// A vertically aligned flight carries the next flight in a multi-storey
	// stairwell. Without this, every generated flight above the ground floor
	// entered the collapse state shortly after spawning.
	for(TActorIterator<AWoodStairs> It(GetWorld());It;++It)
	{
		if(*It==this||It->IsCollapsing())continue;
		if(FMath::Abs(GetActorLocation().Z-(It->GetActorLocation().Z+300.f))<=28.f
			&&FVector::Dist2D(GetActorLocation(),It->GetActorLocation())<=335.f)return true;
	}
	return false;
}

AWoodPillar::AWoodPillar()
{
	MaxStructureHealth=300.f;StructureHealth=300.f;HalfModuleLength=0.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodAsset(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Piece=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarPiece"));Piece->SetupAttachment(SceneRoot);
	if(CubeAsset.Succeeded())Piece->SetStaticMesh(CubeAsset.Object);if(WoodAsset.Succeeded())Piece->SetMaterial(0,WoodAsset.Object);
	Piece->SetCollisionProfileName(TEXT("BlockAll"));Piece->SetRelativeLocation(FVector(0.f,0.f,150.f));Piece->SetRelativeScale3D(FVector(.28f,.28f,3.f));
}

AWallTorch::AWallTorch()
{
	MaxStructureHealth=80.f;StructureHealth=80.f;HalfModuleLength=0.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TorchMesh(TEXT("/Game/ThirdPersonBP/Build/Meshes/Wall_mounted_torch/wall_mounted_torch.wall_mounted_torch"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem> FireParticle(TEXT("/Game/StarterContent/Particles/P_Fire.P_Fire"));
	static ConstructorHelpers::FObjectFinder<USoundCue> FireSound(TEXT("/Game/StarterContent/Audio/Fire01_Cue.Fire01_Cue"));
	Pole=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TorchPole"));Pole->SetupAttachment(SceneRoot);
	if(TorchMesh.Succeeded())
	{
		// Preserve the authored proportions. Independent XYZ fitting distorted the
		// bracket and made the torch look as if it had been mounted sideways.
		Pole->SetStaticMesh(TorchMesh.Object);const FBoxSphereBounds Bounds=TorchMesh.Object->GetBounds();
		const float UniformScale=105.f/FMath::Max(Bounds.BoxExtent.Z*2.f,KINDA_SMALL_NUMBER);
		// Put the rearmost point of the imported mounting plate exactly at local
		// X=0. The actor origin can then sit on the timber surface while the whole
		// torch extends outward along local +X instead of floating or clipping.
		const float SourceBackX=Bounds.Origin.X-Bounds.BoxExtent.X;
		Pole->SetRelativeScale3D(FVector(UniformScale));Pole->SetRelativeLocation(FVector(-SourceBackX*UniformScale,-Bounds.Origin.Y*UniformScale,-Bounds.Origin.Z*UniformScale));
	}
	// The imported wall plate is authored in the actor's Y/Z plane. Keep its
	// local orientation fixed; only the actor yaw is used to select a wall side.
	Pole->SetRelativeRotation(FRotator(0.f,-90.f,0.f));
	Pole->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bracket=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallBracket"));Bracket->SetupAttachment(SceneRoot);
	Bracket->SetVisibility(false);Bracket->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flame=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flame"));Flame->SetupAttachment(SceneRoot);
	Flame->SetVisibility(false);Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FireEffect=CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireEffect"));FireEffect->SetupAttachment(SceneRoot);
	FireEffect->SetRelativeLocation(FVector(11.f,0.f,42.f));FireEffect->SetRelativeScale3D(FVector(.16f));FireEffect->SetAutoActivate(true);FireEffect->SetCastShadow(false);
	if(FireParticle.Succeeded())FireEffect->SetTemplate(FireParticle.Object);
	FireAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudio"));FireAudio->SetupAttachment(SceneRoot);FireAudio->SetRelativeLocation(FVector(8.f,0.f,45.f));FireAudio->bAutoActivate=true;FireAudio->SetVolumeMultiplier(.28f);FireAudio->SetPitchMultiplier(.92f);
	if(FireSound.Succeeded())FireAudio->SetSound(FireSound.Object);
	TorchLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("TorchLight"));TorchLight->SetupAttachment(SceneRoot);
	TorchLight->SetRelativeLocation(FVector(18.f,0.f,44.f));TorchLight->SetLightColor(FLinearColor(1.f,.24f,.035f));TorchLight->SetIntensity(1250.f);TorchLight->SetAttenuationRadius(560.f);
	// Generated interiors can contain dozens of torches. Shadowed movable point
	// lights render six shadow views each and were the direct source of the red
	// "too many overlapping shadowed lights" warning and severe frame drops.
	TorchLight->SetCastShadows(false);
}

void AWallTorch::BeginPlay()
{
	Super::BeginPlay();
	UpdateTorchLight();
}

void AWallTorch::Tick(float DeltaSeconds){Super::Tick(DeltaSeconds);UpdateTorchLight();}

void AWallTorch::UpdateTorchLight()
{
	const bool bLit=!bConstructionPreview&&!bCollapsing;
	if(TorchLight)
	{
		TorchLight->SetVisibility(bLit,true);
		if(bLit&&GetWorld())
		{
			const float Now=GetWorld()->GetTimeSeconds();if(Now>=NextFlickerTime){NextFlickerTime=Now+FMath::FRandRange(.045f,.13f);FlickerTarget=FMath::FRandRange(.82f,1.13f);}
			const float FineFlicker=1.f+FMath::Sin(Now*19.7f)*.035f+FMath::Sin(Now*31.1f)*.018f;
			TorchLight->SetIntensity(1250.f*FlickerTarget*FineFlicker);
		}
	}
	if(FireEffect){FireEffect->SetVisibility(bLit,true);if(bLit&&!FireEffect->IsActive())FireEffect->ActivateSystem();else if(!bLit&&FireEffect->IsActive())FireEffect->DeactivateSystem();}
	if(FireAudio){FireAudio->SetVisibility(bLit,true);if(bLit&&!FireAudio->IsPlaying())FireAudio->Play();else if(!bLit&&FireAudio->IsPlaying())FireAudio->Stop();}
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
