#include "HealthArmorComponent.h"
#include "Net/UnrealNetwork.h"

UHealthArmorComponent::UHealthArmorComponent() { SetIsReplicatedByDefault(true); }
void UHealthArmorComponent::BeginPlay() { Super::BeginPlay(); if (GetOwner()->HasAuthority()) Health = MaxHealth; }
float UHealthArmorComponent::ApplyDamage(float RawDamage, AController*, AActor*)
{
	if (!GetOwner()->HasAuthority() || IsDead() || RawDamage <= 0.f) return 0.f;
	const float Applied = RawDamage * (1.f - FMath::Clamp(ArmorReduction, 0.f, .9f));
	const float Old = Health; Health = FMath::Clamp(Health - Applied, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, Health - Old); if (IsDead()) OnDeath.Broadcast(); return Old - Health;
}
void UHealthArmorComponent::OnRep_Health(float OldHealth) { OnHealthChanged.Broadcast(Health, Health - OldHealth); if (IsDead() && OldHealth > 0.f) OnDeath.Broadcast(); }
void UHealthArmorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(UHealthArmorComponent, Health); DOREPLIFETIME(UHealthArmorComponent, ArmorReduction); }
