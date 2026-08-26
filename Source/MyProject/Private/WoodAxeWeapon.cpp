#include "WoodAxeWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"

AWoodAxeWeapon::AWoodAxeWeapon()
{
	WeaponName=TEXT("WOOD AXE");bAutomatic=false;bMeleeWeapon=true;ProjectileClass=nullptr;
	EquippedSocketOverride=TEXT("two_hands_weapon");
	Stats.RoundsPerMinute=72.f;MeleeRange=245.f;MeleeRadius=34.f;MeleeDamage=58.f;WoodDamageMultiplier=4.5f;MeleeDismembermentHitPower=2;
	Stats.MagazineSize=0;AmmoInMagazine=0;ReserveAmmo=0;MaxReserveAmmo=0;
	AxeMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WoodAxeMesh"));AxeMesh->SetupAttachment(Mesh);
	AxeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);AxeMesh->SetRelativeLocation(FVector::ZeroVector);
	AxeMesh->SetRelativeRotation(FRotator(0.f,0.f,0.f));AxeMesh->SetRelativeScale3D(FVector::OneVector);
	static ConstructorHelpers::FObjectFinder<UStaticMesh>Axe(TEXT("/Game/ThirdPersonBP/Player_0/Weapon/WoodAxe/SM_WoodAxe.SM_WoodAxe"));
	if(Axe.Succeeded())
	{
		AxeMesh->SetStaticMesh(Axe.Object);
		// The source mesh was authored in a much larger unit scale. Normalize the
		// equipped version just like the pickup, targeting a 115 cm overall length.
		const FVector Size=Axe.Object->GetBounds().BoxExtent*2.f;
		const float LongestDimension=FMath::Max3(Size.X,Size.Y,Size.Z);
		if(LongestDimension>KINDA_SMALL_NUMBER)
		{
			const float AxeScale=FMath::Clamp(115.f/LongestDimension,0.05f,20.f);
			AxeMesh->SetRelativeScale3D(FVector(AxeScale));
		}
	}
	// Do not rely on an old imported mesh slot saved by the editor: Blueprint
	// children and cooked builds receive the same authored PBR material explicitly.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>AxeMaterial(TEXT("/Game/ThirdPersonBP/Player_0/Weapon/WoodAxe/M_WoodAxeColored.M_WoodAxeColored"));if(AxeMaterial.Succeeded())AxeMesh->SetMaterial(0,AxeMaterial.Object);
	// These imports are retargeted to UE4_Mannequin_Skeleton, which is also used by
	// the playable character.  Do not reference the similarly named Person_0
	// originals: those have a different hierarchy and produce a T-pose.
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AxeIdle(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Idle_Anim_mixamo_com.Great_Sword_Idle_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AxeWalk(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Walk_Anim_mixamo_com.Great_Sword_Walk_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AxeRun(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Run_Anim_mixamo_com.Great_Sword_Run_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AxeStrafe(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Strafe_Anim_mixamo_com.Great_Sword_Strafe_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AxeJump(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Two_Hands_weapon/Great_Sword_Jump_Anim_mixamo_com.Great_Sword_Jump_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AxeSlash(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Great_Sword_Slash_Anim_mixamo_com.Great_Sword_Slash_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AxeInward(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Stable_Sword_Inward_Slash_Anim_mixamo_com.Stable_Sword_Inward_Slash_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AxeOutward(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Stable_Sword_Outward_Slash_Anim_mixamo_com.Stable_Sword_Outward_Slash_Anim_mixamo_com"));
	CharacterMeleeIdleAnimation=AxeIdle.Object;
	CharacterMeleeWalkAnimation=AxeWalk.Object;
	CharacterMeleeRunAnimation=AxeRun.Object;
	CharacterMeleeStrafeAnimation=AxeStrafe.Object;
	CharacterMeleeJumpAnimation=AxeJump.Object;
	if(AxeSlash.Succeeded())CharacterMeleeAttackAnimations.Add(AxeSlash.Object);
	if(AxeInward.Succeeded())CharacterMeleeAttackAnimations.Add(AxeInward.Object);
	if(AxeOutward.Succeeded())CharacterMeleeAttackAnimations.Add(AxeOutward.Object);
	bUseMeleeLocomotionAnimations=AxeIdle.Succeeded()&&AxeWalk.Succeeded();
}
