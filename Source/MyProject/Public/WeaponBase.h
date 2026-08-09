#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterTypes.h"
#include "WeaponBase.generated.h"
class USkeletalMeshComponent; class ABallisticProjectile;class UNiagaraSystem;class USoundBase;class UAnimMontage;
UCLASS(Blueprintable)
class MYPROJECT_API AWeaponBase : public AActor
{
	GENERATED_BODY()
public:
	AWeaponBase();virtual void Tick(float DeltaSeconds)override; virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	UFUNCTION(BlueprintCallable) bool Fire(FVector AimDirection);
	UFUNCTION(BlueprintCallable) void Reload();
	UFUNCTION(BlueprintPure) bool CanFire()const;
	UFUNCTION(BlueprintCallable) int32 AddReserveAmmo(int32 Amount);
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) USkeletalMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) USkeletalMeshComponent* FirstPersonMesh;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) FWeaponStats Stats;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Weapon")FString WeaponName=TEXT("Weapon");
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
	UFUNCTION(BlueprintPure) FVector GetMuzzleLocation()const;
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
protected:
	UFUNCTION(Server,Reliable,WithValidation) void ServerFire(FVector_NetQuantizeNormal AimDirection);
	UFUNCTION(Server,Reliable,WithValidation) void ServerReload();
	UFUNCTION(NetMulticast,Unreliable)void MulticastFireEffects(FVector_NetQuantize MuzzleLocation,FRotator MuzzleRotation);
	bool FireAuthoritative(FVector AimDirection);void FinishReload();double LastFireTime=-1000.;FTimerHandle ReloadTimer;
};
