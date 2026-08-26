#include "WeaponPickup.h"
#include "StarterRifle.h"
#include "KA47Rifle.h"
#include "SMG11Weapon.h"
#include "ASValRifle.h"
#include "P9Weapon.h"
#include "AK74UWeapon.h"
#include "WoodAxeWeapon.h"
#include "ShooterCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

AWeaponPickup::AWeaponPickup()
{
	WeaponClass=AStarterRifle::StaticClass();
	ItemId=TEXT("AR4");
	PhysicsBounds=CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponPhysicsBounds"));
	SetRootComponent(PhysicsBounds);
	PhysicsBounds->SetBoxExtent(FVector(55.f,18.f,12.f));
	PhysicsBounds->SetCollisionProfileName(TEXT("PhysicsActor"));
	PhysicsBounds->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsBounds->SetEnableGravity(true);
	PhysicsBounds->SetSimulatePhysics(true);
	PhysicsBounds->SetLinearDamping(1.5f);
	PhysicsBounds->SetAngularDamping(2.f);

	Mesh->SetupAttachment(PhysicsBounds);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetEnableGravity(false);
	InteractionRange->SetupAttachment(PhysicsBounds);

	static ConstructorHelpers::FObjectFinder<UStaticMesh>PickupMesh(TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/AR4/SM_AR4.SM_AR4"));
	if(PickupMesh.Succeeded())Mesh->SetStaticMesh(PickupMesh.Object);
	Mesh->SetRelativeScale3D(FVector(1.f));
}
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
	else if(C==AWoodAxeWeapon::StaticClass())
	{
		M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ThirdPersonBP/Player_0/Weapon/WoodAxe/SM_WoodAxe.SM_WoodAxe"));
		ItemId=TEXT("WoodAxe");
	}
	else
	{
		M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/AR4/SM_AR4.SM_AR4"));
		ItemId=TEXT("AR4");
	}
	if(M)
	{
		Mesh->SetStaticMesh(M);
		Mesh->SetRelativeLocation(FVector::ZeroVector);
		Mesh->SetRelativeScale3D(FVector(1.f));
		if(C==AWoodAxeWeapon::StaticClass())
		{
			if(UMaterialInterface* AxeMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/ThirdPersonBP/Player_0/Weapon/WoodAxe/M_WoodAxeColored.M_WoodAxeColored")))
				Mesh->SetMaterial(0,AxeMaterial);
			const FBoxSphereBounds AssetBounds=M->GetBounds();
			const float LongestDimension=FMath::Max(AssetBounds.BoxExtent.GetMax()*2.f,1.f);
			const float AxeScale=FMath::Clamp(115.f/LongestDimension,0.05f,20.f);
			const FVector ScaledExtent=AssetBounds.BoxExtent*AxeScale;
			Mesh->SetRelativeScale3D(FVector(AxeScale));
			// Centre an asset with an offset import pivot inside the physical body.
			Mesh->SetRelativeLocation(-AssetBounds.Origin*AxeScale);
			PhysicsBounds->SetBoxExtent(FVector(
				FMath::Max(ScaledExtent.X,5.f),
				FMath::Max(ScaledExtent.Y,5.f),
				FMath::Max(ScaledExtent.Z,5.f)));
			UE_LOG(LogTemp,Display,TEXT("WoodAxe pickup visual ready: asset extent=%s origin=%s scale=%.3f physics=%s"),
				*AssetBounds.BoxExtent.ToCompactString(),*AssetBounds.Origin.ToCompactString(),AxeScale,*PhysicsBounds->GetUnscaledBoxExtent().ToCompactString());
		}
		else PhysicsBounds->SetBoxExtent(FVector(55.f,18.f,12.f));
	}
	else UE_LOG(LogTemp,Error,TEXT("Weapon pickup mesh failed to load for %s"),*GetNameSafe(C));

	// The box owns world physics; the imported mesh is deliberately visual-only.
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetSimulatePhysics(false);
	PhysicsBounds->SetCollisionProfileName(TEXT("PhysicsActor"));
	PhysicsBounds->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsBounds->SetEnableGravity(true);
	PhysicsBounds->SetSimulatePhysics(true);
	PhysicsBounds->WakeAllRigidBodies();
}
void AWeaponPickup::OnRep_WeaponClass(){ConfigureWeaponClass(WeaponClass);}
void AWeaponPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWeaponPickup,WeaponClass);}
bool AWeaponPickup::TryPickup(APawn* P){if(!HasAuthority()||!P||FVector::DistSquared(P->GetActorLocation(),GetActorLocation())>FMath::Square(400.f))return false;if(AShooterCharacter*C=Cast<AShooterCharacter>(P)){C->EquipWeapon(WeaponClass);Destroy();return true;}return false;}
