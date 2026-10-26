#pragma once

#include "CoreMinimal.h"
#include "ZombieCharacter.h"
#include "BruteZombieCharacter.generated.h"

UCLASS(Blueprintable)
class MYPROJECT_API ABruteZombieCharacter : public AZombieCharacter
{
	GENERATED_BODY()
public:
	ABruteZombieCharacter();
protected:
	virtual float GetMeleeDamage(const UHealthArmorComponent* TargetHealth)const override;
	virtual FVector GetMeleeLaunchVelocity(const FVector& AttackDirection,bool bKick)const override;
};
