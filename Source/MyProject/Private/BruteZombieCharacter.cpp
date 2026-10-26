#include "BruteZombieCharacter.h"

#include "HealthArmorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

ABruteZombieCharacter::ABruteZombieCharacter()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BruteMesh(TEXT("/Game/ThirdPersonBP/Bot/Brute/t-pose.t-pose"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> BruteIdle(TEXT("/Game/ThirdPersonBP/Bot/Brute/Anim/brute_idle_Anim.brute_idle_Anim"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> BruteWalk(TEXT("/Game/ThirdPersonBP/Bot/Brute/Anim/brute_walking_Anim.brute_walking_Anim"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> BruteAttack00(TEXT("/Game/ThirdPersonBP/Bot/Brute/Anim/brute_attack_00_Anim.brute_attack_00_Anim"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> BruteAttack01(TEXT("/Game/ThirdPersonBP/Bot/Brute/Anim/brut_attac_01_Anim.brut_attac_01_Anim"));
	if(BruteMesh.Succeeded())GetMesh()->SetSkeletalMesh(BruteMesh.Object);
	if(BruteIdle.Succeeded()){IdleAnimation=BruteIdle.Object;WakeAnimation=BruteIdle.Object;}
	if(BruteWalk.Succeeded())WalkAnimation=BruteWalk.Object;
	if(BruteAttack00.Succeeded())AttackAnimation=BruteAttack00.Object;
	if(BruteAttack01.Succeeded())AttackAnimation2=BruteAttack01.Object;
	GetCapsuleComponent()->SetCapsuleSize(58.f,125.f);
	GetMesh()->SetRelativeLocation(FVector(0.f,0.f,-125.f));
	GetMesh()->SetRelativeScale3D(FVector(1.25f));
	bUseVisualVariant=false;
	VariantBodyColor=FLinearColor(.55f,.68f,1.f,1.f);
	VariantPantsColor=FLinearColor(.3f,.38f,.62f,1.f);
	VariantTopColor=FLinearColor(.48f,.58f,.86f,1.f);
	GetCharacterMovement()->MaxWalkSpeed=225.f;
	GetCharacterMovement()->RotationRate=FRotator(0.f,180.f,0.f);
	WalkAnimationPlayRate=1.05f;
	MaxCombatHealth=480.f;
	AttackDamage=25.f;
	AttackRange=235.f;
	AttackCooldown=2.5f;
	AttackPlayRate=1.05f;
	BiteChance=0.f;
	KickChance=0.f;
	BiteAnimation=nullptr;
	KickAnimation=nullptr;
	HeadDetachImpulse=48000.f;
	KillExperience=75;
	HeadshotBonusExperience=25;
	LootBagDropChance=.55f;
	ArmDetachHits=7;
	LegDetachHits=9;
	CrawlSpeed=75.f;
	if(Health)Health->MaxHealth=4800.f;
}

float ABruteZombieCharacter::GetMeleeDamage(const UHealthArmorComponent* TargetHealth)const
{
	return TargetHealth?TargetHealth->MaxHealth*.25f:AttackDamage;
}

FVector ABruteZombieCharacter::GetMeleeLaunchVelocity(const FVector& AttackDirection,bool)const
{
	return AttackDirection*650.f+FVector(0.f,0.f,115.f);
}
