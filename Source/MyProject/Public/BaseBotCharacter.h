#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseBotCharacter.generated.h"

UCLASS(Blueprintable)
class MYPROJECT_API ABaseBotCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseBotCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Получает любой тип урона, включая Apply Point Damage.
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	// Максимальное здоровье бота.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bot|Health",
		meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	// Текущее здоровье.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bot|Health")
	float CurrentHealth = 100.0f;

	// Показывает, умер ли бот.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bot|Health")
	bool bIsDead = false;

	UFUNCTION(BlueprintPure, Category = "Bot|Health")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "Bot|Health")
	float GetHealthPercent() const;

	// Вызывается в Blueprint при попадании Point Damage.
	UFUNCTION(BlueprintImplementableEvent, Category = "Bot|Damage")
	void BP_OnPointDamageReceived(
		float Damage,
		FVector HitLocation,
		FVector ShotDirection,
		FName HitBone,
		AActor* DamageCauser
	);

	// Вызывается в Blueprint после получения любого урона.
	UFUNCTION(BlueprintImplementableEvent, Category = "Bot|Damage")
	void BP_OnDamageReceived(
		float Damage,
		AActor* DamageCauser
	);

	// Вызывается в Blueprint один раз при смерти.
	UFUNCTION(BlueprintImplementableEvent, Category = "Bot|Health")
	void BP_OnDeath(
		AController* KillerController,
		AActor* DamageCauser
	);

protected:
	virtual void Die(
		AController* KillerController,
		AActor* DamageCauser
	);
};