#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterCharacter.generated.h"
class UCameraComponent;class USpringArmComponent;class UInventoryComponent;class UHealthArmorComponent;class UNavigationInvokerComponent;class UAIPerceptionStimuliSourceComponent;class USkeletalMeshComponent;class UStaticMeshComponent;class UBlendSpace;class UAnimMontage;class UAnimSequence;class UAnimInstance;class AWeaponBase;class APickupActor;
UCLASS(Blueprintable)
class MYPROJECT_API AShooterCharacter:public ACharacter
{
	GENERATED_BODY()
public:
	AShooterCharacter();virtual void BeginPlay()override;virtual void Tick(float DeltaSeconds)override;virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)override;virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) UCameraComponent* Camera;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)USkeletalMeshComponent* FirstPersonArms;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)USkeletalMeshComponent* FirstPersonBody;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* LeftFirstPersonArm;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* RightFirstPersonArm;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation")UBlendSpace* RifleLocomotion=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation")UAnimMontage* FirstPersonFireMontage=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation")UAnimSequence* FirstPersonIdleAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation")UAnimSequence* FirstPersonFireAnimation=nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Sockets")FName CameraSocketName=TEXT("head");
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Sockets")FName EquippedWeaponSocketName=TEXT("sktGun");
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Sockets")FName StowedWeaponSocketName=TEXT("skt_back_1");
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Camera")FVector CameraSocketOffset=FVector(5.6473f,14.f,1.9467f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Camera")FRotator CameraSocketRotation=FRotator(0.f,89.999f,-84.999f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Camera")FVector CameraSocketScale=FVector(.2187f,.25f,.2187f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Camera")bool bHideLocalHead=false;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Camera",meta=(ClampMin="0.0"))float AimCameraPullback=8.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation")TSubclassOf<UAnimInstance> UnarmedAnimClass;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation")TSubclassOf<UAnimInstance> ArmedAnimClass;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation",meta=(ClampMin="-1.0",ClampMax="1.0"))float AimPitchScale=1.f;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) UInventoryComponent* Inventory;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) UHealthArmorComponent* Health;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Navigation")UNavigationInvokerComponent* NavigationInvoker;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="AI")UAIPerceptionStimuliSourceComponent* StimuliSource;
	UPROPERTY(ReplicatedUsing=OnRep_Weapon,VisibleAnywhere,BlueprintReadOnly) AWeaponBase* EquippedWeapon=nullptr;
	UPROPERTY(ReplicatedUsing=OnRep_Weapon,VisibleAnywhere,BlueprintReadOnly) TArray<AWeaponBase*> WeaponSlots;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly) int32 ActiveWeaponSlot=INDEX_NONE;
	UFUNCTION(BlueprintCallable) void EquipWeapon(TSubclassOf<AWeaponBase> WeaponClass);
	UFUNCTION(BlueprintPure) bool IsAiming()const{return bIsAiming;}
	UFUNCTION(BlueprintPure) bool IsDead()const;
protected:
	void MoveForward(float Value);void MoveRight(float Value);void TurnAtRate(float Value);void LookUpAtRate(float Value);void SwitchWeapon(float Value);void StartFire();void StopFire();void FireOnce();void StartAim();void StopAim();void Reload();void Interact();void SprintPressed();void SprintReleased();void ToggleCrouch();
	UFUNCTION(Server,Reliable,WithValidation)void ServerEquipWeapon(TSubclassOf<AWeaponBase> WeaponClass);
	UFUNCTION(Server,Reliable,WithValidation)void ServerSwitchWeapon(int32 Direction);
	UFUNCTION(Server,Reliable,WithValidation)void ServerInteract(APickupActor* Pickup);
	UFUNCTION(Server,Reliable,WithValidation)void ServerSetAiming(bool bNewAiming);
	UFUNCTION()void OnRep_Weapon();
	UFUNCTION()void HandleDeath();
	UPROPERTY(EditDefaultsOnly,Category="Movement")float WalkSpeed=450.f;UPROPERTY(EditDefaultsOnly,Category="Movement")float SprintSpeed=700.f;UPROPERTY(EditDefaultsOnly,Category="Movement")float BaseTurnRate=45.f;UPROPERTY(EditDefaultsOnly,Category="Interaction")float InteractionDistance=300.f;UPROPERTY(EditDefaultsOnly,Category="Aim")float HipFOV=90.f;UPROPERTY(EditDefaultsOnly,Category="Aim")float AimFOV=75.f;UPROPERTY(Replicated)bool bIsAiming=false;FTimerHandle FireTimer;
	void CaptureDiagnosticScreenshot();
	void ResetFirstPersonArmsAnimation();
	FVector ArmsBaseLocation=FVector(-20.f,8.f,-98.f);
	FRotator ArmsBaseRotation=FRotator(0.f,-10.f,0.f);
	FVector ArmsHipLocation=FVector(-20.f,8.f,-98.f);
	FVector ArmsAimLocation=FVector(-20.f,-13.8f,-89.8f);
	FRotator ArmsHipRotation=FRotator(0.f,-10.f,0.f);
	FRotator ArmsAimRotation=FRotator(0.f,-10.f,0.f);
	float RecoilPitchCurrent=0.f,RecoilPitchTarget=0.f,RecoilYawCurrent=0.f,RecoilYawTarget=0.f,RecoilKickCurrent=0.f,RecoilKickTarget=0.f;
};
