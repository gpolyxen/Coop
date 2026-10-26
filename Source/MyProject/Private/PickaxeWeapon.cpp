#include "PickaxeWeapon.h"

#include "Animation/AnimSequence.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APickaxeWeapon::APickaxeWeapon()
{
	WeaponName=TEXT("PICKAXE");bAutomatic=false;bMeleeWeapon=true;ProjectileClass=nullptr;
	EquippedSocketOverride=TEXT("two_hands_weapon");
	Stats.RoundsPerMinute=64.f;MeleeRange=255.f;MeleeRadius=30.f;MeleeDamage=66.f;MeleeStaminaCost=14.f;WoodDamageMultiplier=1.25f;MeleeDismembermentHitPower=2;
	Stats.MagazineSize=0;AmmoInMagazine=0;ReserveAmmo=0;MaxReserveAmmo=0;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Metal(TEXT("/Game/StarterContent/Materials/M_Metal_Burnished_Steel.M_Metal_Burnished_Steel"));
	auto MakePart=[this](const TCHAR* Name,UStaticMesh* PartMesh,UMaterialInterface* Material)
	{
		UStaticMeshComponent* Part=CreateDefaultSubobject<UStaticMeshComponent>(Name);Part->SetupAttachment(Mesh);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetStaticMesh(PartMesh);if(Material)Part->SetMaterial(0,Material);return Part;
	};
	Handle=MakePart(TEXT("PickaxeHandle"),Cylinder.Object,Wood.Object);Head=MakePart(TEXT("PickaxeHead"),Cube.Object,Metal.Object);Point=MakePart(TEXT("PickaxePoint"),Cone.Object,Metal.Object);BackPoint=MakePart(TEXT("PickaxeBackPoint"),Cone.Object,Metal.Object);
	Handle->SetRelativeScale3D(FVector(.055f,.055f,1.12f));
	Head->SetRelativeLocation(FVector(0.f,0.f,55.f));Head->SetRelativeScale3D(FVector(.62f,.10f,.09f));
	Point->SetRelativeLocation(FVector(74.f,0.f,55.f));Point->SetRelativeRotation(FRotator(0.f,90.f,0.f));Point->SetRelativeScale3D(FVector(.10f,.10f,.36f));
	BackPoint->SetRelativeLocation(FVector(-74.f,0.f,55.f));BackPoint->SetRelativeRotation(FRotator(0.f,-90.f,0.f));BackPoint->SetRelativeScale3D(FVector(.10f,.10f,.28f));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Idle_Anim_mixamo_com.Great_Sword_Idle_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Walk(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Walk_Anim_mixamo_com.Great_Sword_Walk_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Run(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Run_Anim_mixamo_com.Great_Sword_Run_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Strafe(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Strafe_Anim_mixamo_com.Great_Sword_Strafe_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Jump(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Jump_Anim_mixamo_com.Great_Sword_Jump_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Strike(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Great_Sword_Slash_Anim_mixamo_com.Great_Sword_Slash_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Strike2(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Stable_Sword_Inward_Slash_Anim_mixamo_com.Stable_Sword_Inward_Slash_Anim_mixamo_com"));
	CharacterMeleeIdleAnimation=Idle.Object;CharacterMeleeWalkAnimation=Walk.Object;CharacterMeleeRunAnimation=Run.Object;CharacterMeleeStrafeAnimation=Strafe.Object;CharacterMeleeJumpAnimation=Jump.Object;
	if(Strike.Succeeded())CharacterMeleeAttackAnimations.Add(Strike.Object);if(Strike2.Succeeded())CharacterMeleeAttackAnimations.Add(Strike2.Object);bUseMeleeLocomotionAnimations=Idle.Succeeded()&&Walk.Succeeded();
}
