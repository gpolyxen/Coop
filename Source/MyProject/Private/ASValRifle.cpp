#include "ASValRifle.h"

#include "Components/SkeletalMeshComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AASValRifle::AASValRifle()
{
	WeaponName=TEXT("AS VAL");
	AimCameraOffset=FVector(0.f,-8.f,1.f);
	WeaponAimFOV=32.f;
	bUseScopeOverlay=true;
	RecoilPitch=.48f;
	RecoilYaw=.12f;
	RecoilKickback=.7f;
	RecoilRecoverySpeed=14.f;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ValMesh(TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/KA_Val/SK_KA_Val_X.SK_KA_Val_X"));
	if(ValMesh.Succeeded())
	{
		Mesh->SetSkeletalMesh(ValMesh.Object);
		FirstPersonMesh->SetSkeletalMesh(ValMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Flash(TEXT("/Game/sA_ShootingVfxPack/FX/NiagaraSystems/NS_AR_Muzzleflash_1_ONCE.NS_AR_Muzzleflash_1_ONCE"));
	if(Flash.Succeeded())MuzzleFlash=Flash.Object;
	static ConstructorHelpers::FObjectFinder<USoundBase> Shot(TEXT("/Game/FirstPerson/Audio/FirstPersonTemplateWeaponFire02.FirstPersonTemplateWeaponFire02"));
	if(Shot.Succeeded())FireSound=Shot.Object;

	Stats.Damage=42.f;
	Stats.HeadshotDamageMultiplier=2.6f;
	Stats.LimbDamageMultiplier=.55f;
	Stats.MagazineSize=20;
	Stats.RoundsPerMinute=800.f;
	Stats.ReloadSeconds=2.6f;
	Stats.MuzzleVelocity=29500.f;
	Stats.SpreadDegrees=.22f;
	Stats.DragCoefficient=.32f;
	Stats.WindInfluence=1.25f;
	Stats.GravityScale=1.f;
	Stats.ProjectileLifeSeconds=18.f;
	Stats.NoiseLoudness=.45f;
	AmmoInMagazine=Stats.MagazineSize;
	ReserveAmmo=80;
	MaxReserveAmmo=120;
}
