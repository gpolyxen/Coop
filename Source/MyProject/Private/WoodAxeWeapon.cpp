#include "WoodAxeWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AWoodAxeWeapon::AWoodAxeWeapon()
{
	WeaponName=TEXT("WOOD AXE");bAutomatic=false;bMeleeWeapon=true;ProjectileClass=nullptr;
	Stats.RoundsPerMinute=72.f;MeleeRange=245.f;MeleeRadius=34.f;MeleeDamage=58.f;WoodDamageMultiplier=4.5f;MeleeDismembermentHitPower=2;
	Stats.MagazineSize=0;AmmoInMagazine=0;ReserveAmmo=0;MaxReserveAmmo=0;
	AxeMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WoodAxeMesh"));AxeMesh->SetupAttachment(Mesh);
	AxeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);AxeMesh->SetRelativeLocation(FVector::ZeroVector);
	AxeMesh->SetRelativeRotation(FRotator(0.f,0.f,0.f));AxeMesh->SetRelativeScale3D(FVector::OneVector);
	static ConstructorHelpers::FObjectFinder<UStaticMesh>Axe(TEXT("/Game/ThirdPersonBP/Player_0/Weapon/WoodAxe/SM_WoodAxe.SM_WoodAxe"));if(Axe.Succeeded())AxeMesh->SetStaticMesh(Axe.Object);
	// Do not rely on an old imported mesh slot saved by the editor: Blueprint
	// children and cooked builds receive the same authored PBR material explicitly.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>AxeMaterial(TEXT("/Game/ThirdPersonBP/Player_0/Weapon/WoodAxe/M_WoodAxeColored.M_WoodAxeColored"));if(AxeMaterial.Succeeded())AxeMesh->SetMaterial(0,AxeMaterial.Object);
	// Mixamo exports two takes per FBX in UE4.27; the *_mixamo_com take contains
	// the actual motion, while Take_001 is only the auxiliary take.
	static ConstructorHelpers::FObjectFinder<UAnimSequence>Attack1(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Axe_Attack_1_mixamo_com.Axe_Attack_1_mixamo_com"));if(Attack1.Succeeded())CharacterMeleeAttackAnimations.Add(Attack1.Object);
	static ConstructorHelpers::FObjectFinder<UAnimSequence>Attack2(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Axe_Attack_2_mixamo_com.Axe_Attack_2_mixamo_com"));if(Attack2.Succeeded())CharacterMeleeAttackAnimations.Add(Attack2.Object);
	static ConstructorHelpers::FObjectFinder<UAnimSequence>Attack3(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Axe_Attack_3_mixamo_com.Axe_Attack_3_mixamo_com"));if(Attack3.Succeeded())CharacterMeleeAttackAnimations.Add(Attack3.Object);
	bUseMeleeLocomotionAnimations=true;
	static ConstructorHelpers::FObjectFinder<UAnimSequence>Idle(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/TwoHand_Idle_mixamo_com.TwoHand_Idle_mixamo_com"));if(Idle.Succeeded())CharacterMeleeIdleAnimation=Idle.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence>Walk(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/TwoHand_Walk_mixamo_com.TwoHand_Walk_mixamo_com"));if(Walk.Succeeded())CharacterMeleeWalkAnimation=Walk.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence>Run(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/TwoHand_Run_mixamo_com.TwoHand_Run_mixamo_com"));if(Run.Succeeded())CharacterMeleeRunAnimation=Run.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence>Strafe(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/TwoHand_Strafe_mixamo_com.TwoHand_Strafe_mixamo_com"));if(Strafe.Succeeded())CharacterMeleeStrafeAnimation=Strafe.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence>Jump(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/TwoHand_Jump_mixamo_com.TwoHand_Jump_mixamo_com"));if(Jump.Succeeded())CharacterMeleeJumpAnimation=Jump.Object;
}
