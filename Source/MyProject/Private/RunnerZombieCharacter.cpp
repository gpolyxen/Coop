#include "RunnerZombieCharacter.h"

#include "HealthArmorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ARunnerZombieCharacter::ARunnerZombieCharacter()
{
	GetCharacterMovement()->MaxWalkSpeed=580.f;
	GetCharacterMovement()->RotationRate=FRotator(0.f,540.f,0.f);
	GetMesh()->SetRelativeScale3D(FVector(.94f,.94f,1.03f));
	WalkAnimationPlayRate=2.15f;
	AttackDamage=22.f;
	AttackCooldown=1.65f;
	AttackPlayRate=1.5f;
	MaxCombatHealth=170.f;
	if(Health)Health->MaxHealth=1700.f;
}
