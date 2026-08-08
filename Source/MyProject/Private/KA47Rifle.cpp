#include "KA47Rifle.h"
#include "Components/SkeletalMeshComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
AKA47Rifle::AKA47Rifle(){WeaponName=TEXT("KA47");FirstPersonCameraRotation=FRotator::ZeroRotator;FirstPersonGripLocation=FVector::ZeroVector;FirstPersonGripRotation=FRotator::ZeroRotator;RecoilPitch=.95f;RecoilYaw=.28f;RecoilKickback=1.35f;RecoilRecoverySpeed=10.f;static ConstructorHelpers::FObjectFinder<USkeletalMesh>FP(TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/Ka47/SK_KA47_X.SK_KA47_X"));if(FP.Succeeded()){Mesh->SetSkeletalMesh(FP.Object);FirstPersonMesh->SetSkeletalMesh(FP.Object);}static ConstructorHelpers::FObjectFinder<USoundBase>AKShot(TEXT("/Game/StarterContent/Audio/Explosion02.Explosion02"));if(AKShot.Succeeded())FireSound=AKShot.Object;Stats.Damage=40;Stats.MagazineSize=30;Stats.RoundsPerMinute=560;Stats.ReloadSeconds=2.7f;Stats.MuzzleVelocity=71500;Stats.SpreadDegrees=.55f;AmmoInMagazine=30;ReserveAmmo=90;MaxReserveAmmo=90;}
