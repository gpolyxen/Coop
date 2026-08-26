#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterTypes.h"
#include "WeaponBase.generated.h"
class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimSequence;
class ABallisticProjectile;
class UNiagaraSystem;
class USoundBase;
class UAnimMontage;
UCLASS(Blueprintable)
class MYPROJECT_API AWeaponBase : public AActor
{
	GENERATED_BODY()
public:
	AWeaponBase();virtual void Tick(float DeltaSeconds)override; virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	// AimPoint is the camera-selected world point. The physical projectile still
	// starts at the muzzle, while close-range weapons may preserve the selected
	// hit bone when muzzle parallax crosses another part of the same target.
	UFUNCTION(BlueprintCallable) bool Fire(FVector AimPoint);
	UFUNCTION(BlueprintCallable) void Reload();
	UFUNCTION(BlueprintPure) bool CanFire()const;
	UFUNCTION(BlueprintCallable) int32 AddReserveAmmo(int32 Amount);
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) USkeletalMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) USkeletalMeshComponent* FirstPersonMesh;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FWeaponStats Stats;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Weapon")FString WeaponName=TEXT("Weapon");
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Weapon")bool bAutomatic=true;
	// Optional character socket used only while this weapon is equipped.
	// NAME_None keeps the character's normal sktGun socket.
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Weapon")FName EquippedSocketOverride=NAME_None;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) TSubclassOf<ABallisticProjectile> ProjectileClass;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Effects")UNiagaraSystem* MuzzleFlash=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Effects")USoundBase* FireSound=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation")UAnimMontage* CharacterFireMontage=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation")UAnimMontage* CharacterAimFireMontage=nullptr;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly) int32 AmmoInMagazine=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly) int32 ReserveAmmo=90;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,meta=(ClampMin="0")) int32 MaxReserveAmmo=120;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly) bool bIsReloading=false;
	UFUNCTION(BlueprintPure) int32 GetTotalAmmo()const{return AmmoInMagazine+ReserveAmmo;}
	UFUNCTION(BlueprintPure) virtual FVector GetMuzzleLocation()const;
	// Optional camera-attached arms + weapon rig. It is only created visually for
	// the locally controlled owner while this exact weapon is equipped.
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")bool bUseDedicatedFirstPersonRig=false;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")bool bHideOwnerCharacterMeshWhenRigActive=true;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")USkeletalMesh* FirstPersonRigMesh=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")UAnimSequence* FirstPersonRigDrawAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")UAnimSequence* FirstPersonRigIdleAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")UAnimSequence* FirstPersonRigWalkAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")UAnimSequence* FirstPersonRigFireAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")UAnimSequence* FirstPersonRigReloadAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")UAnimSequence* FirstPersonRigInspectAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")FVector FirstPersonRigLocation=FVector(0.f,0.f,-155.f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")FRotator FirstPersonRigRotation=FRotator::ZeroRotator;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")FVector FirstPersonRigScale=FVector::OneVector;
	// Material slots that belong to third-person clothing/body geometry and must
	// not obscure the camera in this weapon's dedicated FPS rig.
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig")TArray<int32> HiddenFirstPersonRigMaterialSlots;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig|Aiming")bool bUseFirstPersonRigAimTransform=false;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig|Aiming")FVector FirstPersonRigAimLocation=FVector(0.f,0.f,-155.f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig|Aiming")FRotator FirstPersonRigAimRotation=FRotator::ZeroRotator;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig|Aiming")FVector FirstPersonRigAimScale=FVector::OneVector;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson|Dedicated Rig|Aiming",meta=(ClampMin="1.0"))float FirstPersonRigAimInterpSpeed=18.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson")FVector FirstPersonRestLocation=FVector(45.f,0.f,-22.f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson")FRotator FirstPersonCameraRotation=FRotator(0.f,-90.f,0.f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Recoil")float RecoilPitch=0.7f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Recoil")float RecoilYaw=0.2f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Recoil")float RecoilKickback=1.0f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Recoil")float RecoilRecoverySpeed=12.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson")FVector FirstPersonGripLocation=FVector(2.f,40.f,-4.f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="FirstPerson")FRotator FirstPersonGripRotation=FRotator::ZeroRotator;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Aiming")FVector AimCameraOffset=FVector(0.f,-8.f,0.f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Aiming")FName AimReferenceSocket=TEXT("skt_ads");
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Aiming",meta=(ClampMin="0.0"))float AimEyeRelief=8.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Aiming",meta=(ClampMin="5.0",ClampMax="120.0"))float WeaponAimFOV=75.f;
	// If the mesh has skt_ads, designers can align ADS entirely in the mesh editor.
	// Weapons without that socket keep using the tested per-weapon fallback offset.
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Aiming")bool bUseAimReferenceSocket=true;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Aiming")bool bUseScopeOverlay=false;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Aiming")bool bPlayCharacterAimFireMontage=false;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Aiming",meta=(ClampMin="0.0"))float CloseRangeHitCorrectionDistance=0.f;
	// Server-side guaranteed alert radius. Perception can miss a freshly spawned
	// listener, so every real shot also wakes all zombies inside the active zone.
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="AI",meta=(ClampMin="1000.0"))float GunshotAlertRadius=20000.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee")bool bMeleeWeapon=false;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee",meta=(ClampMin="10.0"))float MeleeRange=230.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee",meta=(ClampMin="0.0"))float MeleeRadius=32.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee",meta=(ClampMin="0.0"))float MeleeDamage=55.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee",meta=(ClampMin="1.0"))float WoodDamageMultiplier=4.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee")TArray<UAnimSequence*> CharacterMeleeAttackAnimations;
	// A melee tool owns its complete authored pose set.  This keeps an axe from
	// being evaluated by the rifle AnimBP merely because it occupies a weapon slot.
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee|Animation")bool bUseMeleeLocomotionAnimations=false;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee|Animation")UAnimSequence* CharacterMeleeIdleAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee|Animation")UAnimSequence* CharacterMeleeWalkAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee|Animation")UAnimSequence* CharacterMeleeRunAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee|Animation")UAnimSequence* CharacterMeleeStrafeAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee|Animation")UAnimSequence* CharacterMeleeJumpAnimation=nullptr;
	// One axe contact counts as several light projectile contacts for severing,
	// while health damage continues to use MeleeDamage normally.
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Melee|Dismemberment",meta=(ClampMin="1"))int32 MeleeDismembermentHitPower=1;
	bool IsMeleeActionAnimationPlaying()const;
protected:
	UFUNCTION(Server,Reliable,WithValidation) void ServerFire(FVector_NetQuantize AimPoint);
	UFUNCTION(Server,Reliable,WithValidation) void ServerReload();
	UFUNCTION(NetMulticast,Unreliable)void MulticastFireEffects(FVector_NetQuantize MuzzleLocation,FRotator MuzzleRotation);
	UFUNCTION(NetMulticast,Unreliable)void MulticastMeleeEffects(int32 AttackIndex);
	bool FireAuthoritative(FVector AimPoint);void FinishReload();double LastFireTime=-1000.;double MeleeActionAnimationUntil=-1000.;FTimerHandle ReloadTimer;int32 LastMeleeAttackIndex=INDEX_NONE;
};
