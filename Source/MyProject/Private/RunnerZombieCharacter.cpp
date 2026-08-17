#include "RunnerZombieCharacter.h"

#include "HealthArmorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ARunnerZombieCharacter::ARunnerZombieCharacter()
{
	GetCharacterMovement()->MaxWalkSpeed=580.f;
	GetCharacterMovement()->RotationRate=FRotator(0.f,540.f,0.f);
	GetMesh()->SetRelativeScale3D(FVector(.82f,.82f,1.12f));
	bUseVisualVariant=true;
	VariantBodyColor=FLinearColor(1.f,.48f,.5f,1.f);
	VariantPantsColor=FLinearColor(.62f,.28f,.32f,1.f);
	VariantTopColor=FLinearColor(1.f,.38f,.42f,1.f);
	WalkAnimationPlayRate=2.15f;
	AttackDamage=22.f;
	AttackCooldown=1.65f;
	AttackPlayRate=1.5f;
	MaxCombatHealth=170.f;
	KillExperience=25;
	HeadshotBonusExperience=15;
	LootBagDropChance=.25f;
	if(Health)Health->MaxHealth=1700.f;
}
