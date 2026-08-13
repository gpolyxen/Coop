#include "BuildableStructure.h"

#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABuildableStructure::ABuildableStructure()
{
	bReplicates=true;SetReplicateMovement(false);SetCanBeDamaged(true);
	SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));RootComponent=SceneRoot;
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
	MaxStructureHealth=350.f;StructureHealth=350.f;
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
	MaxStructureHealth=500.f;StructureHealth=500.f;
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

AWoodStairs::AWoodStairs()
{
	MaxStructureHealth=400.f;StructureHealth=400.f;HalfModuleLength=150.f;
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
