#include "SaveBed.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ASaveBed::ASaveBed()
{
	bReplicates=true;
	SetReplicateMovement(false);
	SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent=SceneRoot;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Cloth(TEXT("/Game/StarterContent/Materials/M_Basic_Wall.M_Basic_Wall"));

	Frame=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	Frame->SetupAttachment(SceneRoot);
	if(Cube.Succeeded())Frame->SetStaticMesh(Cube.Object);
	if(Wood.Succeeded())Frame->SetMaterial(0,Wood.Object);
	Frame->SetRelativeLocation(FVector(0.f,0.f,22.f));
	Frame->SetRelativeScale3D(FVector(2.1f,1.05f,.28f));
	Frame->SetCollisionProfileName(TEXT("BlockAll"));

	Mattress=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mattress"));
	Mattress->SetupAttachment(SceneRoot);
	if(Cube.Succeeded())Mattress->SetStaticMesh(Cube.Object);
	if(Cloth.Succeeded())Mattress->SetMaterial(0,Cloth.Object);
	Mattress->SetRelativeLocation(FVector(0.f,0.f,47.f));
	Mattress->SetRelativeScale3D(FVector(1.95f,.92f,.22f));
	Mattress->SetCollisionProfileName(TEXT("BlockAll"));

	Headboard=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Headboard"));
	Headboard->SetupAttachment(SceneRoot);
	if(Cube.Succeeded())Headboard->SetStaticMesh(Cube.Object);
	if(Wood.Succeeded())Headboard->SetMaterial(0,Wood.Object);
	Headboard->SetRelativeLocation(FVector(-100.f,0.f,95.f));
	Headboard->SetRelativeScale3D(FVector(.16f,1.1f,.95f));
	Headboard->SetCollisionProfileName(TEXT("BlockAll"));

	SaveLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("SaveLight"));
	SaveLight->SetupAttachment(SceneRoot);
	SaveLight->SetRelativeLocation(FVector(0.f,0.f,115.f));
	SaveLight->SetLightColor(FLinearColor(.1f,.55f,1.f));
	SaveLight->SetIntensity(1800.f);
	SaveLight->SetAttenuationRadius(450.f);

	SaveText=CreateDefaultSubobject<UTextRenderComponent>(TEXT("SaveText"));
	SaveText->SetupAttachment(SceneRoot);
	SaveText->SetRelativeLocation(FVector(0.f,0.f,120.f));
	SaveText->SetRelativeRotation(FRotator(0.f,90.f,0.f));
	SaveText->SetHorizontalAlignment(EHTA_Center);
	SaveText->SetWorldSize(25.f);
	SaveText->SetTextRenderColor(FColor(70,180,255));
	SaveText->SetText(FText::FromString(TEXT("E  SAVE")));
	SaveText->bAlwaysRenderAsText=true;
}
