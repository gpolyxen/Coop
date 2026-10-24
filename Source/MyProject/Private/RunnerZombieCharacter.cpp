#include "RunnerZombieCharacter.h"

#include "HealthArmorComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

ARunnerZombieCharacter::ARunnerZombieCharacter()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> RunnerMesh(TEXT("/Game/ThirdPersonBP/Bot/Sprinters/Infected_sprinter_00/sprinter_zombie_rigged.sprinter_zombie_rigged"));
	if(RunnerMesh.Succeeded())GetMesh()->SetSkeletalMesh(RunnerMesh.Object);
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunnerIdle(TEXT("/Game/ThirdPersonBP/Bot/Sprinters/Infected_sprinter_00/Anim/sprinter_idle_01_Anim.sprinter_idle_01_Anim"));
	if(RunnerIdle.Succeeded())IdleAnimation=RunnerIdle.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunnerRun(TEXT("/Game/ThirdPersonBP/Bot/Sprinters/Infected_sprinter_00/Anim/sprinter_run_Anim.sprinter_run_Anim"));
	if(RunnerRun.Succeeded()){RunAnimation=RunnerRun.Object;WalkAnimation=RunnerRun.Object;}
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunnerAttack(TEXT("/Game/ThirdPersonBP/Bot/Sprinters/Infected_sprinter_00/Anim/sprinter_attack_00_Anim.sprinter_attack_00_Anim"));
	if(RunnerAttack.Succeeded())AttackAnimation=RunnerAttack.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunnerAttack2(TEXT("/Game/ThirdPersonBP/Bot/Sprinters/Infected_sprinter_00/Anim/sprinter_attack_01_Anim.sprinter_attack_01_Anim"));
	if(RunnerAttack2.Succeeded())AttackAnimation2=RunnerAttack2.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunnerBite(TEXT("/Game/ThirdPersonBP/Bot/Sprinters/Infected_sprinter_00/Anim/sprinter_bite_Anim.sprinter_bite_Anim"));
	if(RunnerBite.Succeeded())BiteAnimation=RunnerBite.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunnerFall(TEXT("/Game/ThirdPersonBP/Bot/Sprinters/Infected_sprinter_00/Anim/sprinter_fall_after_break_leg_00_Anim.sprinter_fall_after_break_leg_00_Anim"));
	if(RunnerFall.Succeeded())LegBreakFallAnimation=RunnerFall.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunnerCrawl(TEXT("/Game/ThirdPersonBP/Bot/Sprinters/Infected_sprinter_00/Anim/\u0441rawl_Backward_inplace_00.\u0441rawl_Backward_inplace_00"));
	if(RunnerCrawl.Succeeded())CrawlAnimation=RunnerCrawl.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunnerBiteEscape(TEXT("/Game/ThirdPersonBP/Bot/Sprinters/Infected_sprinter_00/Anim/\u0440it_Reaction_after_bite_Anim.\u0440it_Reaction_after_bite_Anim"));
	if(RunnerBiteEscape.Succeeded())
	{
		BiteEscapeReactionAnimation=RunnerBiteEscape.Object;
		HitReactionAnimation=RunnerBiteEscape.Object;
		HitReactionAnimation2=nullptr;
	}
	// Never fall back to sequences authored for the walker's different skeleton.
	WakeAnimation=RunnerIdle.Object;
	TurnAnimation=nullptr;
	KickAnimation=nullptr;
	GetCharacterMovement()->MaxWalkSpeed=580.f;
	GetCharacterMovement()->RotationRate=FRotator(0.f,540.f,0.f);
	GetMesh()->SetRelativeScale3D(FVector(1.f));
	bUseVisualVariant=false;
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
	ArmDetachHits=2;
	LegDetachHits=3;
	if(Health)Health->MaxHealth=1700.f;
}
