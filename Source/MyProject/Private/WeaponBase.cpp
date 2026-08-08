#include "WeaponBase.h"
#include "BallisticProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "ShooterCharacter.h"
#include "UObject/ConstructorHelpers.h"

AWeaponBase::AWeaponBase(){PrimaryActorTick.bCanEverTick=true;bReplicates=true;SetReplicateMovement(true);Mesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WorldWeaponMesh"));RootComponent=Mesh;Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetOwnerNoSee(false);FirstPersonMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonWeaponMesh"));FirstPersonMesh->SetupAttachment(RootComponent);FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);FirstPersonMesh->SetVisibility(false,true);FirstPersonMesh->SetHiddenInGame(true);FirstPersonMesh->CastShadow=false;ProjectileClass=ABallisticProjectile::StaticClass();static ConstructorHelpers::FObjectFinder<UAnimMontage>HipFire(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Montages/Fire_Rifle_Hip_Montage.Fire_Rifle_Hip_Montage"));if(HipFire.Succeeded())CharacterFireMontage=HipFire.Object;static ConstructorHelpers::FObjectFinder<UAnimMontage>AimFire(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Montages/Fire_Rifle_ironsights_Montage.Fire_Rifle_ironsights_Montage"));if(AimFire.Succeeded())CharacterAimFireMontage=AimFire.Object;AmmoInMagazine=Stats.MagazineSize;ReserveAmmo=90;}
void AWeaponBase::Tick(float D){Super::Tick(D);}
FVector AWeaponBase::GetMuzzleLocation()const{static const FName MuzzleBone(TEXT("b_gun_muzzleflash"));return Mesh->DoesSocketExist(MuzzleBone)?Mesh->GetSocketLocation(MuzzleBone):Mesh->GetComponentLocation()+GetActorForwardVector()*100.f;}
bool AWeaponBase::Fire(FVector Aim){if(!HasAuthority()){ServerFire(Aim);return true;}FireAuthoritative(Aim);return true;}
bool AWeaponBase::ServerFire_Validate(FVector_NetQuantizeNormal Aim){return !Aim.IsNearlyZero();}void AWeaponBase::ServerFire_Implementation(FVector_NetQuantizeNormal Aim){FireAuthoritative(Aim);}
void AWeaponBase::FireAuthoritative(FVector Aim){const double Now=GetWorld()->GetTimeSeconds();const double Interval=60./FMath::Max(1.f,Stats.RoundsPerMinute);if(bIsReloading||AmmoInMagazine<=0||Now-LastFireTime<Interval||!ProjectileClass)return;LastFireTime=Now;--AmmoInMagazine;const FVector Muzzle=GetMuzzleLocation();const FVector Shot=FMath::VRandCone(Aim.GetSafeNormal(),FMath::DegreesToRadians(Stats.SpreadDegrees));FActorSpawnParameters P;P.Owner=GetOwner();P.Instigator=Cast<APawn>(GetOwner());ABallisticProjectile* B=GetWorld()->SpawnActor<ABallisticProjectile>(ProjectileClass,Muzzle,Shot.Rotation(),P);if(B){B->InitializeProjectile(Stats.Damage,Stats.DragCoefficient,Stats.WindInfluence,GetOwner()?GetOwner()->GetInstigatorController():nullptr);B->Movement->Velocity=Shot*Stats.MuzzleVelocity;}MulticastFireEffects(Muzzle,Shot.Rotation());UAISense_Hearing::ReportNoiseEvent(GetWorld(),Muzzle,Stats.NoiseLoudness,GetOwner(),4000.f,TEXT("Gunshot"));}
void AWeaponBase::MulticastFireEffects_Implementation(FVector_NetQuantize Muzzle,FRotator Rotation){if(MuzzleFlash&&Mesh)UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlash,Mesh,TEXT("b_gun_muzzleflash"),FVector::ZeroVector,FRotator::ZeroRotator,EAttachLocation::SnapToTarget,true,true,ENCPoolMethod::None,true);if(FireSound)UGameplayStatics::PlaySoundAtLocation(this,FireSound,Muzzle);if(ACharacter*C=Cast<ACharacter>(GetOwner()))if(C->GetMesh()&&C->GetMesh()->GetAnimInstance()){UAnimMontage*Montage=CharacterFireMontage;if(const AShooterCharacter*S=Cast<AShooterCharacter>(C))if(S->IsAiming()&&CharacterAimFireMontage)Montage=CharacterAimFireMontage;if(Montage)C->GetMesh()->GetAnimInstance()->Montage_Play(Montage);}}
void AWeaponBase::Reload(){if(HasAuthority())ServerReload_Implementation();else ServerReload();}bool AWeaponBase::ServerReload_Validate(){return true;}void AWeaponBase::ServerReload_Implementation(){if(bIsReloading||AmmoInMagazine>=Stats.MagazineSize||ReserveAmmo<=0)return;bIsReloading=true;GetWorldTimerManager().SetTimer(ReloadTimer,this,&AWeaponBase::FinishReload,FMath::Max(.1f,Stats.ReloadSeconds),false);}
void AWeaponBase::FinishReload(){if(!HasAuthority())return;const int32 Need=Stats.MagazineSize-AmmoInMagazine,Take=FMath::Min(Need,ReserveAmmo);AmmoInMagazine+=Take;ReserveAmmo-=Take;bIsReloading=false;}
void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWeaponBase,AmmoInMagazine);DOREPLIFETIME(AWeaponBase,ReserveAmmo);DOREPLIFETIME(AWeaponBase,bIsReloading);}
