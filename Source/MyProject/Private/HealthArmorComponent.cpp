#include "HealthArmorComponent.h"
#include "ShooterCharacter.h"
#include "Net/UnrealNetwork.h"

UHealthArmorComponent::UHealthArmorComponent() { SetIsReplicatedByDefault(true); }
void UHealthArmorComponent::BeginPlay() { Super::BeginPlay(); if (GetOwner()->HasAuthority()) Health = MaxHealth; }
float UHealthArmorComponent::ApplyDamage(float RawDamage, AController*, AActor*)
{
	if (!GetOwner()->HasAuthority() || IsDead() || RawDamage <= 0.f) return 0.f;
	AShooterCharacter* Player=Cast<AShooterCharacter>(GetOwner());
	if(Player&&Player->IsLastLifeInvulnerable())return 0.f;
	const float Applied = RawDamage * (1.f - FMath::Clamp(ArmorReduction, 0.f, .9f));
	const float Old = Health;
	float NewHealth=FMath::Clamp(Health-Applied,0.f,MaxHealth);
	if(NewHealth<=0.f&&Player&&Player->TryActivateLastLife())NewHealth=1.f;
	Health=NewHealth;
	OnHealthChanged.Broadcast(Health, Health - Old); if (IsDead()) OnDeath.Broadcast(); return Old - Health;
}
float UHealthArmorComponent::Heal(float Amount)
{
	if (!GetOwner()->HasAuthority() || IsDead() || Amount <= 0.f) return 0.f;
	const float Old = Health;
	Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
	const float Restored = Health - Old;
	if (Restored > 0.f) OnHealthChanged.Broadcast(Health, Restored);
	return Restored;
}
void UHealthArmorComponent::OnRep_Health(float OldHealth) { OnHealthChanged.Broadcast(Health, Health - OldHealth); if (IsDead() && OldHealth > 0.f) OnDeath.Broadcast(); }
void UHealthArmorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(UHealthArmorComponent, MaxHealth); DOREPLIFETIME(UHealthArmorComponent, Health); DOREPLIFETIME(UHealthArmorComponent, ArmorReduction); }
