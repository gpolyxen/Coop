#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieCharacter.generated.h"
class UHealthArmorComponent;
UCLASS(Blueprintable)
class MYPROJECT_API AZombieCharacter:public ACharacter
{
	GENERATED_BODY()
public:
	AZombieCharacter();
	UFUNCTION(BlueprintCallable)bool TryAttack(AActor* Target);
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UHealthArmorComponent* Health;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)float AttackDamage=18.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)float AttackRange=150.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)float AttackCooldown=1.2f;
private:double LastAttackTime=-1000.;UFUNCTION()void HandleDeath();
};
