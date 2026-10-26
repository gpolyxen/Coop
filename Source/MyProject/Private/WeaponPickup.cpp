#include "WeaponPickup.h"
#include "StarterRifle.h"
#include "KA47Rifle.h"
#include "SMG11Weapon.h"
#include "ASValRifle.h"
#include "P9Weapon.h"
#include "AK74UWeapon.h"
#include "WoodAxeWeapon.h"
#include "PickaxeWeapon.h"
#include "ShooterCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
	SkeletalPickupMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalPickupMesh"));SkeletalPickupMesh->SetupAttachment(PhysicsBounds);SkeletalPickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);SkeletalPickupMesh->SetVisibility(false,true);
	InteractionRange->SetupAttachment(PhysicsBounds);
	auto CreatePickupPart=[this](const TCHAR* Name){UStaticMeshComponent* Part=CreateDefaultSubobject<UStaticMeshComponent>(Name);Part->SetupAttachment(PhysicsBounds);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetVisibility(false,true);return Part;};
	PickupPartA=CreatePickupPart(TEXT("PickupPartA"));PickupPartB=CreatePickupPart(TEXT("PickupPartB"));PickupPartC=CreatePickupPart(TEXT("PickupPartC"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh>PickupMesh(TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/AR4/SM_AR4.SM_AR4"));
	if(PickupMesh.Succeeded())Mesh->SetStaticMesh(PickupMesh.Object);
	Mesh->SetRelativeScale3D(FVector(1.f));
}
void AWeaponPickup::ConfigureWeaponClass(TSubclassOf<AWeaponBase> C)
{
	WeaponClass=C;
	Mesh->SetVisibility(true,true);SkeletalPickupMesh->SetVisibility(false,true);
	PickupPartA->SetVisibility(false,true);PickupPartB->SetVisibility(false,true);PickupPartC->SetVisibility(false,true);
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
		ItemId=TEXT("WoodAxe");
		if(USkeletalMesh* Axe=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ThirdPersonBP/Items/Axe_00/first_axe.first_axe")))
		{
			SkeletalPickupMesh->SetSkeletalMesh(Axe);Mesh->SetVisibility(false,true);SkeletalPickupMesh->SetVisibility(true,true);
			const FBoxSphereBounds AssetBounds=Axe->GetBounds();const float LongestDimension=FMath::Max(AssetBounds.BoxExtent.GetMax()*2.f,1.f);const float AxeScale=FMath::Clamp(115.f/LongestDimension,0.05f,20.f);const FVector ScaledExtent=AssetBounds.BoxExtent*AxeScale;
			SkeletalPickupMesh->SetRelativeScale3D(FVector(AxeScale));SkeletalPickupMesh->SetRelativeLocation(-AssetBounds.Origin*AxeScale);SkeletalPickupMesh->SetRelativeRotation(FRotator::ZeroRotator);PhysicsBounds->SetBoxExtent(FVector(FMath::Max(ScaledExtent.X,5.f),FMath::Max(ScaledExtent.Y,5.f),FMath::Max(ScaledExtent.Z,5.f)));
		}
	}
	else if(C==APickaxeWeapon::StaticClass())
	{
		UStaticMesh* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));UStaticMesh* Cone=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cone.Cone"));
		UMaterialInterface* Wood=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));UMaterialInterface* Metal=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/StarterContent/Materials/M_Metal_Burnished_Steel.M_Metal_Burnished_Steel"));
		M=Cylinder;ItemId=TEXT("Pickaxe");Mesh->SetMaterial(0,Wood);Mesh->SetRelativeScale3D(FVector(.055f,.055f,1.12f));
		PickupPartA->SetStaticMesh(Cube);PickupPartA->SetMaterial(0,Metal);PickupPartA->SetRelativeLocation(FVector(0.f,0.f,55.f));PickupPartA->SetRelativeScale3D(FVector(.62f,.10f,.09f));PickupPartA->SetVisibility(true,true);
		PickupPartB->SetStaticMesh(Cone);PickupPartB->SetMaterial(0,Metal);PickupPartB->SetRelativeLocation(FVector(74.f,0.f,55.f));PickupPartB->SetRelativeRotation(FRotator(0.f,90.f,0.f));PickupPartB->SetRelativeScale3D(FVector(.10f,.10f,.36f));PickupPartB->SetVisibility(true,true);
		PickupPartC->SetStaticMesh(Cone);PickupPartC->SetMaterial(0,Metal);PickupPartC->SetRelativeLocation(FVector(-74.f,0.f,55.f));PickupPartC->SetRelativeRotation(FRotator(0.f,-90.f,0.f));PickupPartC->SetRelativeScale3D(FVector(.10f,.10f,.28f));PickupPartC->SetVisibility(true,true);PhysicsBounds->SetBoxExtent(FVector(82.f,14.f,62.f));
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
		if(C!=APickaxeWeapon::StaticClass())Mesh->SetRelativeScale3D(FVector(1.f));
		if(C!=APickaxeWeapon::StaticClass())PhysicsBounds->SetBoxExtent(FVector(55.f,18.f,12.f));
	}
	else if(C!=AWoodAxeWeapon::StaticClass())UE_LOG(LogTemp,Error,TEXT("Weapon pickup mesh failed to load for %s"),*GetNameSafe(C));

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
