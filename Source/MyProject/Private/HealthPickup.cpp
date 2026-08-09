#include "HealthPickup.h"

#include "HealthArmorComponent.h"
#include "ShooterCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHealthPickup::AHealthPickup()
{
	ItemId=TEXT("Health");
	Quantity=35;
	InteractionRange->SetSphereRadius(220.f);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetEnableGravity(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Chrome(TEXT("/Game/StarterContent/Materials/M_Metal_Chrome.M_Metal_Chrome"));
	if(Cube.Succeeded())Mesh->SetStaticMesh(Cube.Object);
	if(Chrome.Succeeded())Mesh->SetMaterial(0,Chrome.Object);
	Mesh->SetRelativeScale3D(FVector(.16f,.38f,.14f));

	CrossBar=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HealthCrossBar"));
	CrossBar->SetupAttachment(RootComponent);
	CrossBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if(Cube.Succeeded())CrossBar->SetStaticMesh(Cube.Object);
	if(Chrome.Succeeded())CrossBar->SetMaterial(0,Chrome.Object);
	CrossBar->SetRelativeScale3D(FVector(2.35f,.43f,1.f));

	MarkerLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("HealthMarkerLight"));
	MarkerLight->SetupAttachment(RootComponent);
	MarkerLight->SetRelativeLocation(FVector(0.f,0.f,45.f));
	MarkerLight->SetLightColor(FLinearColor(1.f,.02f,.02f));
	MarkerLight->SetIntensity(4500.f);
	MarkerLight->SetAttenuationRadius(550.f);

	MarkerText=CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthMarkerText"));
	MarkerText->SetupAttachment(RootComponent);
	MarkerText->SetRelativeLocation(FVector(0.f,0.f,70.f));
	MarkerText->SetRelativeRotation(FRotator(0.f,90.f,0.f));
	MarkerText->SetHorizontalAlignment(EHTA_Center);
	MarkerText->SetWorldSize(30.f);
	MarkerText->SetTextRenderColor(FColor::Red);
	MarkerText->SetText(FText::FromString(TEXT("+ HP")));
	MarkerText->bAlwaysRenderAsText=true;

	RotationMovement=CreateDefaultSubobject<URotatingMovementComponent>(TEXT("HealthRotation"));
	RotationMovement->RotationRate=FRotator(0.f,45.f,0.f);
}

bool AHealthPickup::TryPickup(APawn* Pawn)
{
	if(!HasAuthority()||!Pawn||!InteractionRange->IsOverlappingActor(Pawn))return false;
	if(AShooterCharacter* Character=Cast<AShooterCharacter>(Pawn))
	{
		if(Character->Health&&Character->Health->Heal(static_cast<float>(Quantity))>0.f)
		{
			UE_LOG(LogTemp,Display,TEXT("Player %s picked up %d health"),*Character->GetName(),Quantity);
			Destroy();
			return true;
		}
	}
	return false;
}
