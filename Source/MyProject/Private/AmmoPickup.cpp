#include "AmmoPickup.h"

#include "ShooterCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AAmmoPickup::AAmmoPickup()
{
	ItemId=TEXT("RifleAmmo");
	Quantity=45;
	InteractionRange->SetSphereRadius(220.f);
	Mesh->SetLinearDamping(1.5f);
	Mesh->SetAngularDamping(3.f);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AmmoMesh(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	if(AmmoMesh.Succeeded())Mesh->SetStaticMesh(AmmoMesh.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AmmoMaterial(TEXT("/Game/StarterContent/Materials/M_Metal_Gold.M_Metal_Gold"));
	if(AmmoMaterial.Succeeded())Mesh->SetMaterial(0,AmmoMaterial.Object);
	Mesh->SetRelativeScale3D(FVector(.34f,.24f,.16f));

	MarkerLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("AmmoMarkerLight"));
	MarkerLight->SetupAttachment(RootComponent);
	MarkerLight->SetRelativeLocation(FVector(0.f,0.f,45.f));
	MarkerLight->SetLightColor(FLinearColor(1.f,.65f,.05f));
	MarkerLight->SetIntensity(3500.f);
	MarkerLight->SetAttenuationRadius(500.f);

	MarkerText=CreateDefaultSubobject<UTextRenderComponent>(TEXT("AmmoMarkerText"));
	MarkerText->SetupAttachment(RootComponent);
	MarkerText->SetRelativeLocation(FVector(0.f,0.f,65.f));
	MarkerText->SetRelativeRotation(FRotator(0.f,90.f,0.f));
	MarkerText->SetHorizontalAlignment(EHTA_Center);
	MarkerText->SetWorldSize(28.f);
	MarkerText->SetTextRenderColor(FColor(255,190,20));
	MarkerText->SetText(FText::FromString(TEXT("AMMO")));
	MarkerText->bAlwaysRenderAsText=true;

}

bool AAmmoPickup::TryPickup(APawn* Pawn)
{
	if(!HasAuthority()||!Pawn||FVector::DistSquared(Pawn->GetActorLocation(),GetActorLocation())>FMath::Square(400.f))return false;
	if(AShooterCharacter* Character=Cast<AShooterCharacter>(Pawn))
	{
		const int32 Accepted=Character->AddAmmunition(Quantity);
		if(Accepted>0)
		{
			UE_LOG(LogTemp,Display,TEXT("Player %s picked up %d rounds"),*Character->GetName(),Accepted);
			Destroy();
			return true;
		}
	}
	return false;
}
