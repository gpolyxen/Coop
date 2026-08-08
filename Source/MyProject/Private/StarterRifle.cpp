#include "StarterRifle.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Animation/AnimMontage.h"

AStarterRifle::AStarterRifle()
{
	WeaponName=TEXT("AR4");
	FirstPersonCameraRotation=FRotator::ZeroRotator;
	RecoilPitch=.65f;RecoilYaw=.18f;RecoilKickback=.9f;RecoilRecoverySpeed=13.f;
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> RifleMesh(TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/AR4/SK_AR4.SK_AR4"));
	if(RifleMesh.Succeeded())Mesh->SetSkeletalMesh(RifleMesh.Object);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FirstPersonRifleMesh(TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/AR4/SK_AR4_X.SK_AR4_X"));
	if(FirstPersonRifleMesh.Succeeded()){Mesh->SetSkeletalMesh(FirstPersonRifleMesh.Object);FirstPersonMesh->SetSkeletalMesh(FirstPersonRifleMesh.Object);}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem>Flash(TEXT("/Game/sA_ShootingVfxPack/FX/NiagaraSystems/NS_AR_Muzzleflash_1_ONCE.NS_AR_Muzzleflash_1_ONCE"));if(Flash.Succeeded())MuzzleFlash=Flash.Object;
	static ConstructorHelpers::FObjectFinder<USoundBase>Shot(TEXT("/Game/FirstPerson/Audio/FirstPersonTemplateWeaponFire02.FirstPersonTemplateWeaponFire02"));if(Shot.Succeeded())FireSound=Shot.Object;
	static ConstructorHelpers::FObjectFinder<UAnimMontage>FireMontage(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Montages/Fire_Rifle_Hip_Montage.Fire_Rifle_Hip_Montage"));if(FireMontage.Succeeded())CharacterFireMontage=FireMontage.Object;
	Stats.Damage=32.f;Stats.MagazineSize=30;Stats.RoundsPerMinute=650.f;Stats.ReloadSeconds=2.25f;Stats.MuzzleVelocity=85000.f;Stats.SpreadDegrees=.35f;
	AmmoInMagazine=Stats.MagazineSize;ReserveAmmo=90;MaxReserveAmmo=120;
}
