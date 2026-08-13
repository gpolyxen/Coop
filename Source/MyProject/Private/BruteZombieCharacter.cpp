#include "BruteZombieCharacter.h"

#include "HealthArmorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABruteZombieCharacter::ABruteZombieCharacter()
{
	GetCapsuleComponent()->SetCapsuleSize(58.f,125.f);
	GetMesh()->SetRelativeLocation(FVector(0.f,0.f,-125.f));
	GetMesh()->SetRelativeScale3D(FVector(1.4f));
	GetCharacterMovement()->MaxWalkSpeed=225.f;
	GetCharacterMovement()->RotationRate=FRotator(0.f,180.f,0.f);
	WalkAnimationPlayRate=1.05f;
	MaxCombatHealth=480.f;
	AttackDamage=34.f;
	AttackRange=235.f;
	AttackCooldown=2.5f;
	AttackPlayRate=1.05f;
	HeadDetachImpulse=48000.f;
	KillExperience=75;
	HeadshotBonusExperience=25;
	LootBagDropChance=.55f;
	if(Health)Health->MaxHealth=4800.f;
}
