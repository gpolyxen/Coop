#pragma once

#include "CoreMinimal.h"
#include "ZombieCharacter.h"
#include "SpitterZombieCharacter.generated.h"

class UPointLightComponent;

UCLASS(Blueprintable)
class MYPROJECT_API ASpitterZombieCharacter : public AZombieCharacter
{
	GENERATED_BODY()
public:
	ASpitterZombieCharacter();
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Variant") UPointLightComponent* AcidGlow;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack") float SpitDamage=16.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Attack") float MeleeRange=210.f;
protected:
	virtual void PerformAttackHit() override;
	virtual uint8 SelectAttackVariant(AActor* Target,uint8 NextVariant,uint8 VariantCount)const override;
};
