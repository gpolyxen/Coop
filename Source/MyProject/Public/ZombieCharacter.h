#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieCharacter.generated.h"

class UAnimSequence;
class UAnimationAsset;
class UHealthArmorComponent;
class UNavigationInvokerComponent;
class UMaterialInterface;

UENUM()
enum class EZombieAttackMode : uint8
{
	None,
	HandStrike,
	Bite
};

UCLASS(Blueprintable)
class MYPROJECT_API AZombieCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZombieCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount,const FDamageEvent& DamageEvent,AController* EventInstigator,AActor* DamageCauser) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable) virtual bool TryAttack(AActor* Target);
	UFUNCTION(BlueprintCallable) void SetDormant(bool bNewDormant);
	UFUNCTION(BlueprintCallable) void WakeUp();
	void RegisterBiteEscapePress(AActor* Victim);
	UFUNCTION(BlueprintPure) bool IsDead() const { return bIsDead; }
	UFUNCTION(BlueprintPure) bool IsAttacking() const { return AttackMode!=EZombieAttackMode::None; }
	UFUNCTION(BlueprintPure) bool IsHitReacting() const { return bHitReacting; }
	UFUNCTION(BlueprintPure) bool IsDormant() const { return bDormant; }
	UFUNCTION(BlueprintPure) bool IsNightEmpowered()const{return bNightEmpowered;}

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) UHealthArmorComponent* Health;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Navigation") UNavigationInvokerComponent* NavigationInvoker;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack") float AttackDamage=18.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack") float AttackRange=180.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack") float AttackCooldown=2.25f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack") float AttackHitDelay=.75f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack") float AttackPlayRate=1.25f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack") float AttackFacingThreshold=.2f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") UAnimSequence* IdleAnimation;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") UAnimSequence* WalkAnimation;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") UAnimSequence* AttackAnimation;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") UAnimSequence* CrawlAnimation;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") UAnimSequence* RunAnimation;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") UAnimSequence* WakeAnimation;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") UAnimSequence* HitReactionAnimation;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") UAnimSequence* TurnAnimation;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") UAnimSequence* BiteAnimation;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") float WalkAnimationPlayRate=1.35f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") float StartWalkingSpeed=28.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") float StopWalkingSpeed=8.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") float LocomotionSmoothingSpeed=7.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Visual Variant") bool bUseVisualVariant=false;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Visual Variant") FLinearColor VariantBodyColor=FLinearColor(.42f,.38f,.3f,1.f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Visual Variant") FLinearColor VariantPantsColor=FLinearColor(.08f,.06f,.04f,1.f);
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Visual Variant") FLinearColor VariantTopColor=FLinearColor(.32f,.3f,.25f,1.f);

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Damage") float HeadDetachImpulse=35000.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Damage|Dismemberment",meta=(ClampMin="1")) int32 ArmDetachHits=3;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Damage|Dismemberment",meta=(ClampMin="1")) int32 LegDetachHits=4;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Damage|Dismemberment",meta=(ClampMin="20")) float CrawlSpeed=105.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Damage",meta=(ClampMin="1.0")) float MaxCombatHealth=100.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Rewards",meta=(ClampMin="0")) int32 KillExperience=10;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Rewards",meta=(ClampMin="0")) int32 HeadshotBonusExperience=10;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack|Bite",meta=(ClampMin="0.0",ClampMax="1.0")) float BiteChance=.28f;
	/** A stationary character this close is bitten instead of receiving a random hand strike. */
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack|Bite",meta=(ClampMin="1.0")) float ImmediateBiteRange=125.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack|Bite",meta=(ClampMin="0.0")) float StationaryBiteSpeed=15.f;
	/** Grapple starts near the contact frame instead of waiting for the hand-strike hit frame. */
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack|Bite",meta=(ClampMin="0.01")) float BiteHitDelay=.18f;
	/** A bite deliberately holds the contact frames longer than a hand strike. */
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack|Bite",meta=(ClampMin="0.1")) float BitePlayRate=.72f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack|Bite",meta=(ClampMin="0.1")) float BiteDamageInterval=.45f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack|Bite",meta=(ClampMin="0.1")) float BiteMinimumDuration=1.4f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack|Bite",meta=(ClampMin="1.0")) float BiteMaximumDuration=5.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack|Bite",meta=(ClampMin="1")) int32 BiteEscapePressesRequired=8;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Loot",meta=(ClampMin="0.0",ClampMax="1.0")) float LootBagDropChance=.18f;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Damage") float CombatHealth=100.f;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Damage") int32 HeadHits=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Damage") int32 TorsoHits=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Damage") int32 LimbHits=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Damage") float LethalProgress=0.f;
	UPROPERTY(ReplicatedUsing=OnRep_SeveredLimbs,VisibleAnywhere,BlueprintReadOnly,Category="Damage|Dismemberment") uint8 SeveredLimbs=0;
	UFUNCTION(BlueprintPure,Category="Damage|Dismemberment") bool IsCrawling()const{return (SeveredLimbs&12)!=0;}

protected:
	virtual void PerformAttackHit();
	TWeakObjectPtr<AActor> PendingAttackTarget;

private:
	double LastAttackTime=-1000.;
	UPROPERTY(Replicated) bool bIsDead=false;
	UPROPERTY(ReplicatedUsing=OnRep_AttackMode) EZombieAttackMode AttackMode=EZombieAttackMode::None;
	UPROPERTY(ReplicatedUsing=OnRep_Dormant) bool bDormant=false;
	UPROPERTY(Transient) UAnimationAsset* CurrentAnimation=nullptr;
	UPROPERTY() UMaterialInterface* VisualVariantBodyMaterial=nullptr;
	UPROPERTY() UMaterialInterface* VisualVariantClothesMaterial=nullptr;
	bool bLocomotionMoving=false;
	float SmoothedLocomotionSpeed=0.f;
	bool bNightEmpowered=false;
	float BaseMaxCombatHealth=100.f;
	float BaseAttackDamage=18.f;
	float BaseWalkSpeed=270.f;
	float BaseWalkAnimationPlayRate=1.35f;
	float BaseComponentMaxHealth=1000.f;
	float NextNightStateCheck=0.f;
	FTimerHandle AttackHitTimer;
	FTimerHandle AttackFinishTimer;
	FTimerHandle BiteDamageTimer;
	FTimerHandle HitReactionTimer;
	FTimerHandle WakeTimer;
	TWeakObjectPtr<AActor> BiteVictim;
	double BiteStartTime=-1000.;
	int32 BiteEscapePresses=0;
	bool bWakeSequencePlaying=false;
	bool bHitReacting=false;
	float TemporaryAnimationUntil=0.f;
	float LastObservedYaw=0.f;

	static bool IsHeadBone(FName BoneName);
	static bool IsLimbBone(FName BoneName);
	int32 ResolveLimbIndex(FName BoneName)const;
	FName ResolveLimbRootBone(int32 LimbIndex)const;
	void SeverLimb(int32 LimbIndex,FName HitBone,const FVector& ShotDirection,const FVector& HitLocation);
	void ApplySeveredLimbState();
	FName ResolveHeadBone(FName PreferredBone)const;
	void UpdateLocomotionAnimation();
	void ApplyVisualVariant();
	void UpdateNightEmpowerment();
	void ApplyNightEmpowerment(bool bEnable);
	void PlayZombieAnimation(UAnimationAsset* Animation,bool bLooping,float PlayRate);
	void FinishAttack();
	void ApplyBiteDamage();
	void BeginBite(AActor* Victim);
	void EndBite();
	void BeginHitReaction();
	void FinishHitReaction();
	void FinishWakeUp();
	void Die(bool bHeadshot,FName HitBone,const FVector& ShotDirection,const FVector& HitLocation);

	UFUNCTION() void OnRep_AttackMode();
	UFUNCTION() void OnRep_Dormant();
	UFUNCTION() void OnRep_SeveredLimbs();
	UFUNCTION(NetMulticast,Unreliable) void MulticastBloodImpact(FVector_NetQuantize HitLocation,FVector_NetQuantizeNormal ShotDirection,bool bFountain);
	UFUNCTION(NetMulticast,Unreliable) void MulticastHitReaction();
	UFUNCTION(NetMulticast,Reliable) void MulticastSeverLimb(uint8 LimbBit,FName RootBone,FVector Impulse,FVector HitLocation);
	UFUNCTION(NetMulticast,Reliable) void MulticastDie(bool bHeadshot,FName HitBone,FVector Impulse,FVector HitLocation);
	int32 LimbZoneHits[4]={0,0,0,0};
};
