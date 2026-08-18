#include "WeaponPickup.h"
#include "StarterRifle.h"
#include "KA47Rifle.h"
#include "SMG11Weapon.h"
#include "ASValRifle.h"
#include "P9Weapon.h"
#include "AK74UWeapon.h"
#include "ShooterCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

AWeaponPickup::AWeaponPickup(){WeaponClass=AStarterRifle::StaticClass();ItemId=TEXT("AR4");static ConstructorHelpers::FObjectFinder<UStaticMesh>PickupMesh(TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/AR4/SM_AR4.SM_AR4"));if(PickupMesh.Succeeded())Mesh->SetStaticMesh(PickupMesh.Object);Mesh->SetRelativeScale3D(FVector(1.f));}
void AWeaponPickup::ConfigureWeaponClass(TSubclassOf<AWeaponBase> C)
{
	WeaponClass=C;
	UStaticMesh* M=nullptr;
	if(C==AKA47Rifle::StaticClass())
	{
		M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/Ka47/SM_KA47.SM_KA47"));
		ItemId=TEXT("KA47");
	}
	else if(C==ASMG11Weapon::StaticClass())
	{
		M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/SMG11/SM_SMG11_X.SM_SMG11_X"));
		ItemId=TEXT("SMG11");
	}
	else if(C==AASValRifle::StaticClass())
	{
		M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/KA_Val/SM_KA_Val_X.SM_KA_Val_X"));
		ItemId=TEXT("ASVal");
	}
	else if(C==AP9Weapon::StaticClass())
	{
		M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/P9MannyFPS/Meshes/SM_P9.SM_P9"));
		ItemId=TEXT("P9");
	}
	else if(C==AAK74UWeapon::StaticClass())
	{
		M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/AK74UFree/WorldParts/AK74U_Gun.AK74U_Gun"));
		ItemId=TEXT("AK74U");
	}
	else
	{
		M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/AR4/SM_AR4.SM_AR4"));
		ItemId=TEXT("AR4");
	}
	if(M)Mesh->SetStaticMesh(M);
}
void AWeaponPickup::OnRep_WeaponClass(){ConfigureWeaponClass(WeaponClass);}
void AWeaponPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWeaponPickup,WeaponClass);}
bool AWeaponPickup::TryPickup(APawn* P){if(!HasAuthority()||!P||FVector::DistSquared(P->GetActorLocation(),GetActorLocation())>FMath::Square(400.f))return false;if(AShooterCharacter*C=Cast<AShooterCharacter>(P)){C->EquipWeapon(WeaponClass);Destroy();return true;}return false;}
