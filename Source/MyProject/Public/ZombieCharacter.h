#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieCharacter.generated.h"

class UAnimSequence;
class UAnimationAsset;
class UHealthArmorComponent;
class UNavigationInvokerComponent;

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

	UFUNCTION(BlueprintCallable) bool TryAttack(AActor* Target);
	UFUNCTION(BlueprintPure) bool IsDead() const { return bIsDead; }
	UFUNCTION(BlueprintPure) bool IsAttacking() const { return bIsAttacking; }

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
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") float WalkAnimationPlayRate=1.35f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") float StartWalkingSpeed=28.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") float StopWalkingSpeed=8.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Animation") float LocomotionSmoothingSpeed=7.f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Damage") float HeadDetachImpulse=35000.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Damage",meta=(ClampMin="1.0")) float MaxCombatHealth=100.f;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Damage") float CombatHealth=100.f;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Damage") int32 HeadHits=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Damage") int32 TorsoHits=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Damage") int32 LimbHits=0;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Damage") float LethalProgress=0.f;

private:
	double LastAttackTime=-1000.;
	UPROPERTY(Replicated) bool bIsDead=false;
	UPROPERTY(ReplicatedUsing=OnRep_IsAttacking) bool bIsAttacking=false;
	TWeakObjectPtr<AActor> PendingAttackTarget;
	UPROPERTY(Transient) UAnimationAsset* CurrentAnimation=nullptr;
	bool bLocomotionMoving=false;
	float SmoothedLocomotionSpeed=0.f;
	FTimerHandle AttackHitTimer;
	FTimerHandle AttackFinishTimer;

	static bool IsHeadBone(FName BoneName);
	static bool IsLimbBone(FName BoneName);
	FName ResolveHeadBone(FName PreferredBone)const;
	void UpdateLocomotionAnimation();
	void PlayZombieAnimation(UAnimationAsset* Animation,bool bLooping,float PlayRate);
	void PerformAttackHit();
	void FinishAttack();
	void Die(bool bHeadshot,FName HitBone,const FVector& ShotDirection,const FVector& HitLocation);

	UFUNCTION() void OnRep_IsAttacking();
	UFUNCTION(NetMulticast,Unreliable) void MulticastBloodImpact(FVector_NetQuantize HitLocation,FVector_NetQuantizeNormal ShotDirection,bool bFountain);
	UFUNCTION(NetMulticast,Reliable) void MulticastDie(bool bHeadshot,FName HitBone,FVector Impulse,FVector HitLocation);
};
