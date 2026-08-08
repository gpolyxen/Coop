#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthArmorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHealthChanged, float, Health, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDeathEvent);

UCLASS(ClassGroup=(Shooter), meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UHealthArmorComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UHealthArmorComponent();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintCallable) float ApplyDamage(float RawDamage, AController* InstigatorController, AActor* DamageCauser);
	UFUNCTION(BlueprintPure) bool IsDead() const { return Health <= 0.f; }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health") float MaxHealth = 100.f;
	UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, BlueprintReadOnly, Category="Health") float Health = 100.f;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category="Armor", meta=(ClampMin="0.0", ClampMax="0.9")) float ArmorReduction = 0.f;
	UPROPERTY(BlueprintAssignable) FHealthChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable) FDeathEvent OnDeath;
private:
	UFUNCTION() void OnRep_Health(float OldHealth);
};
