#include "LootSuitcasePickup.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALootSuitcasePickup::ALootSuitcasePickup()
{
	ItemId=TEXT("LootSuitcase");
	MinWood=4;MaxWood=10;MinRope=2;MaxRope=6;
	MedicineChance=.72f;BandageChance=.85f;MaxBandages=4;
	LeatherChance=.65f;MaxLeather=4;ClothChance=.72f;MaxCloth=4;GasolineChance=.45f;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if(Cube.Succeeded())Mesh->SetStaticMesh(Cube.Object);
	if(BasicMaterial.Succeeded())Mesh->SetMaterial(0,BasicMaterial.Object);
	Mesh->SetRelativeScale3D(FVector(.42f,.62f,.20f));
	BagTop->SetVisibility(false,true);RopeTie->SetVisibility(false,true);StickA->SetVisibility(false,true);StickB->SetVisibility(false,true);
	Handle=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SuitcaseHandle"));Handle->SetupAttachment(Mesh);
	if(Cylinder.Succeeded())Handle->SetStaticMesh(Cylinder.Object);if(BasicMaterial.Succeeded())Handle->SetMaterial(0,BasicMaterial.Object);
	Handle->SetRelativeLocation(FVector(0.f,0.f,72.f));Handle->SetRelativeRotation(FRotator(90.f,0.f,0.f));Handle->SetRelativeScale3D(FVector(.13f,.13f,.24f));
	Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupText->SetText(FText::FromString(TEXT("E  ЧЕМОДАН С ПРИПАСАМИ")));
	PickupText->SetRelativeLocation(FVector(0.f,0.f,105.f));
}

void ALootSuitcasePickup::BeginPlay()
{
	Super::BeginPlay();
	if(UMaterialInstanceDynamic* Material=Mesh->CreateAndSetMaterialInstanceDynamic(0))Material->SetVectorParameterValue(TEXT("Color"),FLinearColor(.055f,.09f,.075f));
	if(UMaterialInstanceDynamic* Material=Handle->CreateAndSetMaterialInstanceDynamic(0))Material->SetVectorParameterValue(TEXT("Color"),FLinearColor(.025f,.03f,.025f));
}
