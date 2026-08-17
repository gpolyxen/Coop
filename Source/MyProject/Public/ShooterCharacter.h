#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterTypes.h"
#include "BuildTypes.h"
#include "ShooterCharacter.generated.h"
class UCameraComponent;class USpringArmComponent;class USceneComponent;class UInventoryComponent;class UHealthArmorComponent;class UNavigationInvokerComponent;class UAIPerceptionStimuliSourceComponent;class USkeletalMeshComponent;class UStaticMeshComponent;class UBlendSpace;class UAnimMontage;class UAnimSequence;class UAnimInstance;class AWeaponBase;class APickupActor;class ASaveBed;class AStorageChest;
UCLASS(Blueprintable)
class MYPROJECT_API AShooterCharacter:public ACharacter
{
	GENERATED_BODY()
public:
	AShooterCharacter();virtual void BeginPlay()override;virtual void PossessedBy(AController* NewController)override;virtual void OnRep_Controller()override;virtual void Tick(float DeltaSeconds)override;virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)override;virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) UCameraComponent* Camera;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)USceneComponent* FirstPersonRigRoot;
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
	UFUNCTION(BlueprintCallable) int32 AddAmmunition(int32 Amount);
	UFUNCTION(BlueprintPure) bool CanCraftBed()const;
	UFUNCTION(BlueprintPure) bool CanCraftMedkit()const;
	UFUNCTION(BlueprintCallable) void CraftMedkit();
	UFUNCTION(BlueprintCallable) void UseMedkit();
	UFUNCTION(BlueprintCallable)void TransferItemToChest(AStorageChest* Chest,FName ItemId,int32 Quantity=1);
	UFUNCTION(BlueprintCallable)void TransferItemFromChest(AStorageChest* Chest,FName ItemId,int32 Quantity=1);
	UFUNCTION(BlueprintCallable)void StoreEquippedWeaponInChest(AStorageChest* Chest);
	UFUNCTION(BlueprintPure) bool IsBuildingBed()const{return SelectedBuildPiece==EBuildPieceType::Bed;}
	UFUNCTION(BlueprintPure) bool IsBuilding()const{return SelectedBuildPiece!=EBuildPieceType::None;}
	UFUNCTION(BlueprintPure) bool IsBuildPreviewValid()const{return bBuildPreviewValid;}
	UFUNCTION(BlueprintPure) FString GetSelectedBuildName()const;
	UFUNCTION(BlueprintCallable) void BeginBuildPlacement(EBuildPieceType PieceType);
	UFUNCTION(BlueprintCallable) void AddExperience(int32 Amount);
	UFUNCTION(BlueprintCallable) bool PurchaseSkill(EShooterSkill Skill);
	UFUNCTION(BlueprintPure) bool HasSkill(EShooterSkill Skill)const{return UnlockedSkills.Contains(Skill);}
	UFUNCTION(BlueprintPure) bool CanPurchaseSkill(EShooterSkill Skill)const;
	UFUNCTION(BlueprintPure) int32 GetSkillCost(EShooterSkill Skill)const;
	UFUNCTION(BlueprintPure) FText GetSkillName(EShooterSkill Skill)const;
	UFUNCTION(BlueprintPure) FText GetSkillDescription(EShooterSkill Skill)const;
	UFUNCTION(BlueprintPure) FText GetSkillRequirementText(EShooterSkill Skill)const;
	UFUNCTION(BlueprintPure) float GetReloadTimeMultiplier()const{return HasSkill(EShooterSkill::QuickReload)?.72f:1.f;}
	UFUNCTION(BlueprintPure) float GetRecoilMultiplier()const{return HasSkill(EShooterSkill::SteadyAim)?.78f:1.f;}
	UFUNCTION(BlueprintPure) float GetPickupMultiplier()const{return HasSkill(EShooterSkill::Scavenger)?1.25f:1.f;}
	UFUNCTION(BlueprintPure) float GetHealingMultiplier()const{return HasSkill(EShooterSkill::CombatMedic)?1.4f:1.f;}
	UFUNCTION(BlueprintPure) bool IsLastLifeInvulnerable()const;
	bool TryActivateLastLife();
	void ApplyUnlockedSkillEffects();
	void StopGameplayActionsForMenu();
	void SetActiveWeaponSlotForLoad(int32 SlotIndex);
	void ShowLocalNotification(const FString& Message,float Duration=4.f);
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="UI")FString LocalNotification;
	float LocalNotificationEndTime=0.f;
	UFUNCTION(BlueprintPure) bool IsAiming()const{return bIsAiming;}
	UFUNCTION(BlueprintPure) bool IsDead()const;
	UFUNCTION(BlueprintPure) int32 GetExperienceForNextLevel()const{return BaseExperiencePerLevel+FMath::Max(0,CharacterLevel-1)*ExperienceGrowthPerLevel;}
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Progression")int32 CharacterLevel=1;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Progression")int32 Experience=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Progression")int32 TotalExperience=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Progression")int32 SkillPoints=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Progression")TArray<EShooterSkill> UnlockedSkills;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Progression")bool bLastLifeConsumed=false;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Progression")float LastLifeInvulnerableUntil=0.f;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Stamina")float Stamina=100.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Stamina")float MaxStamina=100.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Stamina")float SprintStaminaPerSecond=18.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Stamina")float JumpStaminaCost=20.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Stamina")float StaminaRecoveryPerSecond=16.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Stamina")float StaminaRecoveryDelay=1.25f;
	UFUNCTION(BlueprintPure)float GetStaminaRatio()const{return MaxStamina>0.f?FMath::Clamp(Stamina/MaxStamina,0.f,1.f):0.f;}
protected:
	void RestoreLocalGameplayInput();
	void MoveForward(float Value);void MoveRight(float Value);void TurnAtRate(float Value);void LookUpAtRate(float Value);void SwitchWeapon(float Value);void StartFire();void StopFire();void FireOnce();void StartAim();void StopAim();void Reload();void Interact();void SprintPressed();void SprintReleased();void JumpPressed();void JumpReleased();void ToggleCrouch();void UpdateBuildPreview();void ConfirmBuildPlacement();void CancelBuildMode();void RotateBuildPreview();
	UFUNCTION(Server,Reliable,WithValidation)void ServerEquipWeapon(TSubclassOf<AWeaponBase> WeaponClass);
	UFUNCTION(Server,Reliable,WithValidation)void ServerSwitchWeapon(int32 Direction);
	UFUNCTION(Server,Reliable,WithValidation)void ServerInteract(APickupActor* Pickup);
	UFUNCTION(Server,Reliable,WithValidation)void ServerUseBed(ASaveBed* Bed);
	UFUNCTION(Server,Reliable,WithValidation)void ServerToggleGate(class AWoodGate* Gate);
	UFUNCTION(Client,Reliable)void ClientSaveAtBed();
	UFUNCTION(Server,Reliable,WithValidation)void ServerSetAiming(bool bNewAiming);
	UFUNCTION(Server,Reliable,WithValidation)void ServerSetSprinting(bool bNewSprinting);
	UFUNCTION(Server,Reliable,WithValidation)void ServerTryJump();
	UFUNCTION(Server,Reliable,WithValidation)void ServerPlaceBuildPiece(EBuildPieceType PieceType,FVector_NetQuantize Location,FRotator Rotation);
	UFUNCTION(Client,Reliable)void ClientBuildPlacementResult(EBuildPieceType PieceType,bool bPlaced,bool bCanContinue);
	UFUNCTION(Server,Reliable,WithValidation)void ServerPurchaseSkill(EShooterSkill Skill);
	UFUNCTION(Server,Reliable,WithValidation)void ServerCraftMedkit();
	UFUNCTION(Server,Reliable,WithValidation)void ServerUseMedkit();
	UFUNCTION(Server,Reliable,WithValidation)void ServerTransferItemToChest(AStorageChest* Chest,FName ItemId,int32 Quantity);
	UFUNCTION(Server,Reliable,WithValidation)void ServerTransferItemFromChest(AStorageChest* Chest,FName ItemId,int32 Quantity);
	UFUNCTION(Server,Reliable,WithValidation)void ServerStoreEquippedWeaponInChest(AStorageChest* Chest);
	UFUNCTION()void OnRep_Weapon();
	UFUNCTION()void HandleDeath();
	void RefreshAnimationState();
	UPROPERTY(EditDefaultsOnly,Category="Movement")float WalkSpeed=450.f;UPROPERTY(EditDefaultsOnly,Category="Movement")float SprintSpeed=700.f;UPROPERTY(EditDefaultsOnly,Category="Movement")float BaseTurnRate=45.f;UPROPERTY(EditDefaultsOnly,Category="Interaction")float InteractionDistance=300.f;UPROPERTY(EditDefaultsOnly,Category="Aim")float HipFOV=90.f;UPROPERTY(EditDefaultsOnly,Category="Aim")float AimFOV=75.f;UPROPERTY(Replicated)bool bIsAiming=false;FTimerHandle FireTimer;
	UPROPERTY(Replicated)bool bWantsToSprint=false;
	float LastStaminaUseTime=-1000.f;
	UPROPERTY(EditDefaultsOnly,Category="Progression",meta=(ClampMin="1"))int32 BaseExperiencePerLevel=100;
	UPROPERTY(EditDefaultsOnly,Category="Progression",meta=(ClampMin="0"))int32 ExperienceGrowthPerLevel=50;
	void CaptureDiagnosticScreenshot();
	void ResetFirstPersonArmsAnimation();
	void ConfigureDedicatedFirstPersonRig();
	void UpdateDedicatedFirstPersonRigTransform(float DeltaSeconds);
	void UpdateDedicatedFirstPersonRigAnimation();
	void PlayDedicatedFirstPersonRigAction(UAnimSequence* Animation);
	void FinishDedicatedFirstPersonRigAction();
	bool IsDedicatedFirstPersonRigActive()const;
	UPROPERTY(Transient)UAnimSequence* CurrentFirstPersonRigAnimation=nullptr;
	UPROPERTY(Transient)AWeaponBase* FirstPersonRigWeapon=nullptr;
	FTimerHandle FirstPersonRigAnimationTimer;
	bool bFirstPersonRigActionPlaying=false;
	bool bFirstPersonRigLooping=false;
	FVector ArmsBaseLocation=FVector(-20.f,8.f,-98.f);
	FRotator ArmsBaseRotation=FRotator(0.f,-10.f,0.f);
	FVector ArmsHipLocation=FVector(-20.f,8.f,-98.f);
	FVector ArmsAimLocation=FVector(-20.f,-13.8f,-89.8f);
	FRotator ArmsHipRotation=FRotator(0.f,-10.f,0.f);
	FRotator ArmsAimRotation=FRotator(0.f,-10.f,0.f);
	float RecoilPitchCurrent=0.f,RecoilPitchTarget=0.f,RecoilYawCurrent=0.f,RecoilYawTarget=0.f,RecoilKickCurrent=0.f,RecoilKickTarget=0.f;
	EBuildPieceType SelectedBuildPiece=EBuildPieceType::None;
	bool bBuildPreviewValid=false;
	int32 BuildRotationQuarterTurns=0;
	UPROPERTY(Transient)AActor* BuildPreview=nullptr;
};
