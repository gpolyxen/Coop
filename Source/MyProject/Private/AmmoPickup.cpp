#include "AmmoPickup.h"

#include "ShooterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AAmmoPickup::AAmmoPickup()
{
	ItemId=TEXT("RifleAmmo");
	Quantity=45;
	InteractionRange->SetSphereRadius(170.f);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AmmoMesh(TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/Ammunition/SM_Shell_762x39.SM_Shell_762x39"));
	if(AmmoMesh.Succeeded())Mesh->SetStaticMesh(AmmoMesh.Object);
	Mesh->SetRelativeScale3D(FVector(3.f));
}

bool AAmmoPickup::TryPickup(APawn* Pawn)
{
	if(!HasAuthority()||!Pawn||!InteractionRange->IsOverlappingActor(Pawn))return false;
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
