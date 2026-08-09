#include "P9Weapon.h"

#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AP9Weapon::AP9Weapon()
{
	WeaponName=TEXT("P9");
	bAutomatic=false;
	bUseDedicatedFirstPersonRig=true;
	bHideOwnerCharacterMeshWhenRigActive=true;
	// The source FPS package uses +Y as its forward axis. Rotate it into Unreal's
	// camera-facing +X axis and keep the whole rig slightly in front of the near plane.
	FirstPersonRigLocation=FVector(18.f,0.f,-155.f);
	FirstPersonRigRotation=FRotator(0.f,-90.f,0.f);
	FirstPersonRigScale=FVector::OneVector;
	bUseFirstPersonRigAimTransform=true;
	// ADS moves the rear/front sights onto the screen centre and drops the
	// forearms below the sight picture instead of moving the camera into them.
	FirstPersonRigAimLocation=FVector(18.f,-2.5f,-156.5f);
	FirstPersonRigAimRotation=FRotator(0.f,-90.f,0.f);
	FirstPersonRigAimScale=FVector(.98f);
	FirstPersonRigAimInterpSpeed=20.f;

	WorldPistolMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("P9WorldMesh"));
	WorldPistolMesh->SetupAttachment(Mesh);
	WorldPistolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorldPistolMesh->SetOwnerNoSee(true);
	// The imported P9 points along local +Y. sktGun expects the weapon along +X.
	WorldPistolMesh->SetRelativeLocation(FVector(5.f,0.f,5.f));
	WorldPistolMesh->SetRelativeRotation(FRotator(0.f,-90.f,0.f));

	MuzzlePoint=CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(WorldPistolMesh);
	MuzzlePoint->SetRelativeLocation(FVector(0.f,10.6f,0.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WorldMesh(
		TEXT("/Game/P9MannyFPS/Meshes/SM_P9.SM_P9"));
	if(WorldMesh.Succeeded())WorldPistolMesh->SetStaticMesh(WorldMesh.Object);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> RigMesh(
		TEXT("/Game/P9MannyFPS/Meshes/SK_P9_MannyFPS.SK_P9_MannyFPS"));
	if(RigMesh.Succeeded())FirstPersonRigMesh=RigMesh.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence> Draw(
		TEXT("/Game/P9MannyFPS/Animations/A_P9_WEP_Draw.A_P9_WEP_Draw"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(
		TEXT("/Game/P9MannyFPS/Animations/A_P9_WEP_Idle.A_P9_WEP_Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Walk(
		TEXT("/Game/P9MannyFPS/Animations/A_P9_WEP_Walk.A_P9_WEP_Walk"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Fire(
		TEXT("/Game/P9MannyFPS/Animations/A_P9_WEP_Fire.A_P9_WEP_Fire"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Reload(
		TEXT("/Game/P9MannyFPS/Animations/A_P9_WEP_Reload_01.A_P9_WEP_Reload_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Inspect(
		TEXT("/Game/P9MannyFPS/Animations/A_P9_WEP_Inspect_01.A_P9_WEP_Inspect_01"));
	if(Draw.Succeeded())FirstPersonRigDrawAnimation=Draw.Object;
	if(Idle.Succeeded())FirstPersonRigIdleAnimation=Idle.Object;
	if(Walk.Succeeded())FirstPersonRigWalkAnimation=Walk.Object;
	if(Fire.Succeeded())FirstPersonRigFireAnimation=Fire.Object;
	if(Reload.Succeeded())FirstPersonRigReloadAnimation=Reload.Object;
	if(Inspect.Succeeded())FirstPersonRigInspectAnimation=Inspect.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> ShotSound(
		TEXT("/Game/FirstPerson/Audio/FirstPersonTemplateWeaponFire02.FirstPersonTemplateWeaponFire02"));
	if(ShotSound.Succeeded())FireSound=ShotSound.Object;
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Flash(
		TEXT("/Game/sA_ShootingVfxPack/FX/NiagaraSystems/NS_AR_Muzzleflash_1_ONCE.NS_AR_Muzzleflash_1_ONCE"));
	if(Flash.Succeeded())MuzzleFlash=Flash.Object;

	Stats.Damage=28.f;
	Stats.MagazineSize=15;
	Stats.RoundsPerMinute=420.f;
	Stats.ReloadSeconds=2.67f;
	Stats.MuzzleVelocity=35000.f;
	Stats.SpreadDegrees=.8f;
	Stats.DragCoefficient=.3f;
	Stats.WindInfluence=.8f;
	Stats.GravityScale=1.f;
	Stats.ProjectileLifeSeconds=8.f;
	AmmoInMagazine=Stats.MagazineSize;
	ReserveAmmo=45;
	MaxReserveAmmo=90;
	RecoilPitch=1.05f;
	RecoilYaw=.3f;
	RecoilKickback=.9f;
	RecoilRecoverySpeed=13.f;
	WeaponAimFOV=75.f;
	bUseAimReferenceSocket=false;
	AimCameraOffset=FVector::ZeroVector;
}

FVector AP9Weapon::GetMuzzleLocation()const
{
	return MuzzlePoint?MuzzlePoint->GetComponentLocation():Super::GetMuzzleLocation();
}
