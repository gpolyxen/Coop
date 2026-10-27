#include "SpitterZombieCharacter.h"

#include "ZombieSpitProjectile.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

ASpitterZombieCharacter::ASpitterZombieCharacter()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SpitterMesh(TEXT("/Game/ThirdPersonBP/Bot/Spitters/Spitter_00/spitter.spitter"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SpitterIdle(TEXT("/Game/ThirdPersonBP/Bot/Spitters/Spitter_00/Anim/idle_spitter_Anim.idle_spitter_Anim"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SpitterWalk(TEXT("/Game/ThirdPersonBP/Bot/Spitters/Spitter_00/Anim/Spitter_Walk_inplace_Anim.Spitter_Walk_inplace_Anim"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SpitterSpit(TEXT("/Game/ThirdPersonBP/Bot/Spitters/Spitter_00/Anim/spit_Anim.spit_Anim"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SpitterMelee(TEXT("/Game/ThirdPersonBP/Bot/Spitters/Spitter_00/Anim/atack_00_Anim.atack_00_Anim"));
	if(SpitterMesh.Succeeded())GetMesh()->SetSkeletalMesh(SpitterMesh.Object);
	if(SpitterIdle.Succeeded()){IdleAnimation=SpitterIdle.Object;WakeAnimation=SpitterIdle.Object;}
	if(SpitterWalk.Succeeded())WalkAnimation=SpitterWalk.Object;
	// Variant zero is the ranged spit, variant one is the close hand strike.
	if(SpitterSpit.Succeeded())AttackAnimation=SpitterSpit.Object;
	if(SpitterMelee.Succeeded())AttackAnimation2=SpitterMelee.Object;
	GetCharacterMovement()->MaxWalkSpeed=250.f;
	GetMesh()->SetRelativeScale3D(FVector(1.03f));
	bUseVisualVariant=false;
	VariantBodyColor=FLinearColor(.58f,1.f,.34f,1.f);
	VariantPantsColor=FLinearColor(.32f,.62f,.22f,1.f);
	VariantTopColor=FLinearColor(.46f,.86f,.28f,1.f);
	AttackRange=1250.f;
	AttackCooldown=3.2f;
	AttackHitDelay=.62f;
	AttackPlayRate=1.1f;
	AttackAnimationEndTrim=.1f;
	MaxCombatHealth=130.f;
	KillExperience=35;
	HeadshotBonusExperience=15;
	LootBagDropChance=.35f;
	ArmDetachHits=4;
	LegDetachHits=5;
	AcidGlow=CreateDefaultSubobject<UPointLightComponent>(TEXT("AcidGlow"));
	AcidGlow->SetupAttachment(RootComponent);
	AcidGlow->SetRelativeLocation(FVector(0.f,0.f,72.f));
	AcidGlow->SetLightColor(FLinearColor(.15f,1.f,.05f));
	AcidGlow->SetIntensity(650.f);
	AcidGlow->SetAttenuationRadius(210.f);
	AcidGlow->SetCastShadows(false);
}

uint8 ASpitterZombieCharacter::SelectAttackVariant(AActor* Target,uint8,uint8 VariantCount)const
{
	if(!Target||VariantCount<2)return 0;
	return FVector::DistSquared2D(GetActorLocation(),Target->GetActorLocation())<=FMath::Square(MeleeRange)?1:0;
}

void ASpitterZombieCharacter::PerformAttackHit()
{
	if(!HasAuthority()||IsDead()||!PendingAttackTarget.IsValid()||!GetWorld())return;
	AActor* Target=PendingAttackTarget.Get();
	if(FVector::DistSquared2D(GetActorLocation(),Target->GetActorLocation())<=FMath::Square(MeleeRange+35.f))
	{
		AZombieCharacter::PerformAttackHit();
		return;
	}
	if(FVector::DistSquared2D(GetActorLocation(),Target->GetActorLocation())>FMath::Square(AttackRange+100.f))return;
	const FVector SpawnLocation=GetActorLocation()+GetActorForwardVector()*55.f+FVector(0.f,0.f,75.f);
	const FVector PredictedTarget=Target->GetActorLocation()+FVector(0.f,0.f,45.f)+Target->GetVelocity()*.22f;
	const FRotator SpawnRotation=(PredictedTarget-SpawnLocation).Rotation();
	FActorSpawnParameters Parameters;
	Parameters.Owner=this;
	Parameters.Instigator=this;
	Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if(AZombieSpitProjectile* Spit=GetWorld()->SpawnActor<AZombieSpitProjectile>(AZombieSpitProjectile::StaticClass(),SpawnLocation,SpawnRotation,Parameters))
	{
		Spit->Damage=SpitDamage;
		Spit->Movement->Velocity=SpawnRotation.Vector()*Spit->Movement->InitialSpeed;
	}
}
