#include "WalkerZombieCharacter.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

AWalkerZombieCharacter::AWalkerZombieCharacter()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> WalkerMesh(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/T-Pose.T-Pose"));
	if(WalkerMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(WalkerMesh.Object);
		GetMesh()->SetRelativeLocation(FVector(0.f,0.f,-90.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f,-90.f,0.f));
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/Zombie_Idle__1__Anim_mixamo_com.Zombie_Idle__1__Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Walk(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/Walking_Anim_mixamo_com.Walking_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Punch(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/Zombie_Punching_Anim_mixamo_com.Zombie_Punching_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Kick(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/Zombie_Kicking_Anim_mixamo_com.Zombie_Kicking_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Bite(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/Zombie_Neck_Bite_Anim_mixamo_com.Zombie_Neck_Bite_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Crawl(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/Zombie_Crawl_Anim_mixamo_com.Zombie_Crawl_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Run(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/Zombie_Running_Anim_mixamo_com.Zombie_Running_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Wake(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/Zombie_Stand_Up_Anim_mixamo_com.Zombie_Stand_Up_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Hit1(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/Zombie_Reaction_Hit_Anim_mixamo_com.Zombie_Reaction_Hit_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Hit2(TEXT("/Game/ThirdPersonBP/Bot/Walkers/Infected_walker_00/Anim/Zombie_Reaction_Hit__2.Zombie_Reaction_Hit__2"));

	if(Idle.Succeeded())IdleAnimation=Idle.Object;
	if(Walk.Succeeded())WalkAnimation=Walk.Object;
	if(Punch.Succeeded())AttackAnimation=Punch.Object;
	if(Kick.Succeeded())KickAnimation=Kick.Object;
	if(Bite.Succeeded())BiteAnimation=Bite.Object;
	if(Crawl.Succeeded())CrawlAnimation=Crawl.Object;
	if(Run.Succeeded())RunAnimation=Run.Object;
	if(Wake.Succeeded())WakeAnimation=Wake.Object;
	if(Hit1.Succeeded())HitReactionAnimation=Hit1.Object;
	if(Hit2.Succeeded())HitReactionAnimation2=Hit2.Object;
	// This pack has no compatible turn clip. Leaving it empty is safer than
	// playing the old animation that belongs to another skeleton.
	TurnAnimation=nullptr;
}
