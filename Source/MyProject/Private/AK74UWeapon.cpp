#include "AK74UWeapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AAK74UWeapon::AAK74UWeapon()
{
	WeaponName=TEXT("AK74U");
	bAutomatic=true;
	// Keep the AK74U as a normal sktGun weapon. The downloaded FPS animation
	// package is intentionally not used; local and remote players share the
	// established third-person character animation path.
	bUseDedicatedFirstPersonRig=false;
	bHideOwnerCharacterMeshWhenRigActive=false;

	// This FBX was authored as a complete first-person scene. Unreal converts
	// its axes on import, so the neutral rig can remain camera-relative here.
	FirstPersonRigLocation=FVector(30.f,0.f,-165.f);
	FirstPersonRigRotation=FRotator(0.f,-90.f,0.f);
	FirstPersonRigScale=FVector::OneVector;
	HiddenFirstPersonRigMaterialSlots.Add(1); // source hoodie has open shoulder geometry unsuitable for FPS
	bUseFirstPersonRigAimTransform=true;
	FirstPersonRigAimLocation=FVector(20.f,-7.5f,-156.5f);
	FirstPersonRigAimRotation=FRotator(0.f,-90.f,0.f);
	FirstPersonRigAimScale=FVector::OneVector;
	FirstPersonRigAimInterpSpeed=18.f;

	WorldGunMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AK74UWorldMesh"));
	WorldGunMesh->SetupAttachment(Mesh);
	WorldGunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorldGunMesh->SetOwnerNoSee(false);
	WorldGunMesh->SetRelativeLocation(FVector::ZeroVector);
	WorldGunMesh->SetRelativeRotation(FRotator(0.f,-90.f,0.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WorldMesh(
		TEXT("/Game/AK74UFree/WorldParts/AK74U_Gun.AK74U_Gun"));
	if(WorldMesh.Succeeded())WorldGunMesh->SetStaticMesh(WorldMesh.Object);

	static ConstructorHelpers::FObjectFinder<USoundBase> ShotSound(
		TEXT("/Game/FirstPerson/Audio/FirstPersonTemplateWeaponFire02.FirstPersonTemplateWeaponFire02"));
	if(ShotSound.Succeeded())FireSound=ShotSound.Object;
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Flash(
		TEXT("/Game/sA_ShootingVfxPack/FX/NiagaraSystems/NS_AR_Muzzleflash_1_ONCE.NS_AR_Muzzleflash_1_ONCE"));
	if(Flash.Succeeded())MuzzleFlash=Flash.Object;

	Stats.Damage=36.f;
	Stats.HeadshotDamageMultiplier=3.f;
	Stats.LimbDamageMultiplier=.55f;
	Stats.MagazineSize=30;
	Stats.RoundsPerMinute=700.f;
	Stats.ReloadSeconds=2.2333f;
	Stats.MuzzleVelocity=73500.f;
	Stats.SpreadDegrees=.48f;
	Stats.DragCoefficient=.24f;
	Stats.WindInfluence=1.f;
	Stats.GravityScale=1.f;
	Stats.ProjectileLifeSeconds=14.f;
	AmmoInMagazine=Stats.MagazineSize;
	ReserveAmmo=90;
	MaxReserveAmmo=120;
	RecoilPitch=.82f;
	RecoilYaw=.24f;
	RecoilKickback=1.15f;
	RecoilRecoverySpeed=11.f;
	WeaponAimFOV=72.f;
	bUseAimReferenceSocket=false;
	AimCameraOffset=FVector::ZeroVector;
}
