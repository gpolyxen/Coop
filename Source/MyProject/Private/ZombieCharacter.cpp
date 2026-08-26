#include "ZombieCharacter.h"

#include "ZombieAIController.h"
#include "BloodBurstActor.h"
#include "BloodPoolActor.h"
#include "HeadGibActor.h"
#include "LimbGibActor.h"
#include "LootBagPickup.h"
#include "BuildableStructure.h"
#include "ShooterCharacter.h"
#include "WeaponBase.h"
#include "BanditCharacter.h"
#include "HealthArmorComponent.h"
#include "OpenWorldStreamingManager.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationInvokerComponent.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AZombieCharacter::AZombieCharacter()
{
	PrimaryActorTick.bCanEverTick=true;
	bReplicates=true;
	AIControllerClass=AZombieAIController::StaticClass();
	AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
	Health=CreateDefaultSubobject<UHealthArmorComponent>(TEXT("Health"));
	Health->MaxHealth=1000.f;
	NavigationInvoker=CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavigationInvoker"));
	NavigationInvoker->SetGenerationRadii(12000.f,16000.f);
	UCharacterMovementComponent* Movement=GetCharacterMovement();
	Movement->MaxWalkSpeed=270.f;
	Movement->bOrientRotationToMovement=true;
	Movement->RotationRate=FRotator(0.f,260.f,0.f);
	Movement->MaxStepHeight=45.f;
	Movement->JumpZVelocity=560.f;
	Movement->AirControl=.55f;
	Movement->GetNavAgentPropertiesRef().bCanJump=true;
	// RVO makes a large horde slow down and orbit outside a narrow player-built
	// entrance.  Zombies receive authored gate/stair lanes from their controller,
	// so local RVO avoidance is counterproductive at those transitions.
	Movement->bUseRVOAvoidance=false;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Ignore);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ZombieMesh(TEXT("/Game/ThirdPersonBP/Bot/bot_animations/T-Pose.T-Pose"));
	if(ZombieMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(ZombieMesh.Object);
		GetMesh()->SetRelativeLocation(FVector(0.f,0.f,-90.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f,-90.f,0.f));
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VariantBodyMaterial(
		TEXT("/Game/ThirdPersonBP/Bot/VariantMaterials/M_ZombieVariant_Body.M_ZombieVariant_Body"));
	if(VariantBodyMaterial.Succeeded())VisualVariantBodyMaterial=VariantBodyMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VariantClothesMaterial(
		TEXT("/Game/ThirdPersonBP/Bot/VariantMaterials/M_ZombieVariant_Clothes.M_ZombieVariant_Clothes"));
	if(VariantClothesMaterial.Succeeded())VisualVariantClothesMaterial=VariantClothesMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAsset(TEXT("/Game/ThirdPersonBP/Bot/bot_animations/Zombie_Idle_Anim.Zombie_Idle_Anim"));
	if(IdleAsset.Succeeded())IdleAnimation=IdleAsset.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAsset(TEXT("/Game/ThirdPersonBP/Bot/bot_animations/Zombie_Walk__1__Anim.Zombie_Walk__1__Anim"));
	if(WalkAsset.Succeeded())WalkAnimation=WalkAsset.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AttackAsset(TEXT("/Game/ThirdPersonBP/Bot/bot_animations/Zombie_Attack__1__Anim.Zombie_Attack__1__Anim"));
	if(AttackAsset.Succeeded())AttackAnimation=AttackAsset.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CrawlAsset(TEXT("/Game/ThirdPersonBP/Bot/bot_animations/Zombie_Crawl_Anim.Zombie_Crawl_Anim"));
	if(CrawlAsset.Succeeded())CrawlAnimation=CrawlAsset.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunAsset(TEXT("/Game/ThirdPersonBP/Bot/bot_animations/Zombie_Run_Anim.Zombie_Run_Anim"));
	if(RunAsset.Succeeded())RunAnimation=RunAsset.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WakeAsset(TEXT("/Game/ThirdPersonBP/Bot/bot_animations/Zombie_Stand_Up_Anim.Zombie_Stand_Up_Anim"));
	if(WakeAsset.Succeeded())WakeAnimation=WakeAsset.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ReactionAsset(TEXT("/Game/ThirdPersonBP/Bot/bot_animations/Zombie_Reaction_Hit_Anim.Zombie_Reaction_Hit_Anim"));
	if(ReactionAsset.Succeeded())HitReactionAnimation=ReactionAsset.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> TurnAsset(TEXT("/Game/ThirdPersonBP/Bot/bot_animations/Zombie_Turn_Anim.Zombie_Turn_Anim"));
	if(TurnAsset.Succeeded())TurnAnimation=TurnAsset.Object;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> BiteAsset(TEXT("/Game/ThirdPersonBP/Bot/bot_animations/Zombie_Neck_Bite_Anim.Zombie_Neck_Bite_Anim"));
	if(BiteAsset.Succeeded())BiteAnimation=BiteAsset.Object;
}

void AZombieCharacter::BeginPlay()
{
	Super::BeginPlay();
	// A horde must not turn into an impassable wall of Pawn capsules at gates and
	// stair landings. Ignore only other zombies while moving; players, terrain and
	// buildable structures keep their normal collision and damage behaviour.
	for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It)
	{
		AZombieCharacter* Other=*It;
		if(!Other||Other==this)continue;
		GetCapsuleComponent()->IgnoreActorWhenMoving(Other,true);
		Other->GetCapsuleComponent()->IgnoreActorWhenMoving(this,true);
	}
	BaseMaxCombatHealth=MaxCombatHealth;
	BaseAttackDamage=AttackDamage;
	BaseWalkSpeed=GetCharacterMovement()->MaxWalkSpeed;
	BaseWalkAnimationPlayRate=WalkAnimationPlayRate;
	BaseComponentMaxHealth=Health?Health->MaxHealth:1000.f;
	if(HasAuthority())CombatHealth=MaxCombatHealth;
	if(HasAuthority()&&!Controller)SpawnDefaultController();
	if(UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld()))
		if(NavigationInvoker)NavigationInvoker->RegisterWithNavigationSystem(*Navigation);
	UE_LOG(LogTemp,Display,TEXT("Zombie %s began play; authority=%s controller=%s"),
		*GetName(),HasAuthority()?TEXT("true"):TEXT("false"),*GetNameSafe(Controller));
	ApplyVisualVariant();
	ApplySeveredLimbState();
	UpdateNightEmpowerment();
	LastObservedYaw=GetActorRotation().Yaw;
	UpdateLocomotionAnimation();
}

void AZombieCharacter::SetDormant(bool bNewDormant)
{
	if(!HasAuthority()||bIsDead)return;
	if(bDormant==bNewDormant)return;
	if(!bNewDormant)
	{
		WakeUp();
		return;
	}
	bDormant=bNewDormant;
	OnRep_Dormant();
}

void AZombieCharacter::WakeUp()
{
	if(!HasAuthority()||bIsDead||!bDormant)return;
	bDormant=false;
	bWakeSequencePlaying=true;
	OnRep_Dormant();
	const float Duration=WakeAnimation?WakeAnimation->GetPlayLength():1.25f;
	GetWorldTimerManager().SetTimer(WakeTimer,this,&AZombieCharacter::FinishWakeUp,FMath::Max(.35f,Duration),false);
}

void AZombieCharacter::OnRep_Dormant()
{
	if(!GetCharacterMovement())return;
	CurrentAnimation=nullptr;
	if(bDormant)
	{
		GetCharacterMovement()->DisableMovement();
		// The first frame of the stand-up take is the authored lying pose. Freeze it
		// there until sight, sound or damage calls WakeUp().
		PlayZombieAnimation(WakeAnimation?WakeAnimation:IdleAnimation,false,1.f);
		if(UAnimSingleNodeInstance* Instance=GetMesh()->GetSingleNodeInstance())
		{
			Instance->SetPosition(0.f,false);
			Instance->SetPlaying(false);
		}
	}
	else if(bWakeSequencePlaying)
	{
		bWakeSequencePlaying=true;
		GetCharacterMovement()->DisableMovement();
		PlayZombieAnimation(WakeAnimation?WakeAnimation:IdleAnimation,false,1.f);
		if(!HasAuthority())
		{
			const float Duration=WakeAnimation?WakeAnimation->GetPlayLength():1.25f;
			GetWorldTimerManager().SetTimer(WakeTimer,this,&AZombieCharacter::FinishWakeUp,FMath::Max(.35f,Duration),false);
		}
	}
}

void AZombieCharacter::FinishWakeUp()
{
	bWakeSequencePlaying=false;
	if(!bIsDead&&!bDormant&&GetCharacterMovement())GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	CurrentAnimation=nullptr;
	UpdateLocomotionAnimation();
}

void AZombieCharacter::ApplyVisualVariant()
{
	if(!bUseVisualVariant||!GetMesh()||!VisualVariantBodyMaterial||!VisualVariantClothesMaterial)return;
	const FLinearColor Colors[]={VariantBodyColor,VariantPantsColor,VariantTopColor};
	const int32 MaterialCount=FMath::Min(GetMesh()->GetNumMaterials(),3);
	for(int32 Slot=0;Slot<MaterialCount;++Slot)
	{
		UMaterialInterface* ParentMaterial=Slot==0?VisualVariantBodyMaterial:VisualVariantClothesMaterial;
		if(UMaterialInstanceDynamic* Material=GetMesh()->CreateDynamicMaterialInstance(Slot,ParentMaterial))
		{
			Material->SetVectorParameterValue(TEXT("TintColor"),Colors[Slot]);
		}
	}
}

void AZombieCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if(bIsDead)return;
	if(bDormant||bWakeSequencePlaying)return;
	if(GetWorld()&&GetWorld()->GetTimeSeconds()>=NextNightStateCheck){NextNightStateCheck=GetWorld()->GetTimeSeconds()+1.f;UpdateNightEmpowerment();}
	SmoothedLocomotionSpeed=FMath::FInterpTo(SmoothedLocomotionSpeed,GetVelocity().Size2D(),DeltaSeconds,LocomotionSmoothingSpeed);
	if(IsAttacking())
	{
		if(HasAuthority()&&PendingAttackTarget.IsValid())
		{
			FVector ToTarget=PendingAttackTarget->GetActorLocation()-GetActorLocation();
			ToTarget.Z=0.f;
			if(!ToTarget.IsNearlyZero())
			{
				const FRotator DesiredRotation=ToTarget.Rotation();
				SetActorRotation(FMath::RInterpTo(GetActorRotation(),DesiredRotation,DeltaSeconds,9.f));
			}
		}
		return;
	}
	const float NewYaw=GetActorRotation().Yaw;
	const float YawDelta=FMath::Abs(FMath::FindDeltaAngleDegrees(LastObservedYaw,NewYaw));
	LastObservedYaw=NewYaw;
	if(!bLocomotionMoving&&SmoothedLocomotionSpeed<StopWalkingSpeed&&YawDelta>8.f&&TurnAnimation&&GetWorld()->GetTimeSeconds()>=TemporaryAnimationUntil)
	{
		TemporaryAnimationUntil=GetWorld()->GetTimeSeconds()+TurnAnimation->GetPlayLength();
		PlayZombieAnimation(TurnAnimation,false,1.f);
	}
	UpdateLocomotionAnimation();
}

void AZombieCharacter::UpdateNightEmpowerment()
{
	if(!GetWorld())return;
	for(TActorIterator<AOpenWorldStreamingManager> It(GetWorld());It;++It)
	{
		ApplyNightEmpowerment(It->IsNightTime());
		return;
	}
}

void AZombieCharacter::ApplyNightEmpowerment(bool bEnable)
{
	if(bNightEmpowered==bEnable)return;
	const float OldMaximum=FMath::Max(1.f,MaxCombatHealth);
	const float HealthRatio=FMath::Clamp(CombatHealth/OldMaximum,0.f,1.f);
	bNightEmpowered=bEnable;
	const float HealthMultiplier=bEnable?1.6f:1.f;
	MaxCombatHealth=BaseMaxCombatHealth*HealthMultiplier;
	AttackDamage=BaseAttackDamage*(bEnable?1.4f:1.f);
	GetCharacterMovement()->MaxWalkSpeed=(IsCrawling()?CrawlSpeed:BaseWalkSpeed)*(bEnable?1.55f:1.f);
	WalkAnimationPlayRate=BaseWalkAnimationPlayRate*(bEnable?1.25f:1.f);
	if(Health)Health->MaxHealth=BaseComponentMaxHealth*HealthMultiplier;
	if(HasAuthority())CombatHealth=MaxCombatHealth*HealthRatio;
	UE_LOG(LogTemp,Display,TEXT("Zombie %s night empowerment %s: HP %.0f, speed %.0f, damage %.0f"),
		*GetName(),bEnable?TEXT("ON"):TEXT("OFF"),MaxCombatHealth,GetCharacterMovement()->MaxWalkSpeed,AttackDamage);
}

void AZombieCharacter::PlayZombieAnimation(UAnimationAsset* Animation,bool bLooping,float PlayRate)
{
	if(!Animation)return;
	const float TargetRate=FMath::Max(.1f,PlayRate);
	if(CurrentAnimation==Animation)
	{
		GetMesh()->GlobalAnimRateScale=FMath::FInterpTo(GetMesh()->GlobalAnimRateScale,TargetRate,
			GetWorld()?GetWorld()->GetDeltaSeconds():0.f,6.f);
		return;
	}
	CurrentAnimation=Animation;
	GetMesh()->GlobalAnimRateScale=TargetRate;
	GetMesh()->PlayAnimation(Animation,bLooping);
}

void AZombieCharacter::UpdateLocomotionAnimation()
{
	if(bIsDead||IsAttacking()||bHitReacting||bDormant||bWakeSequencePlaying||(GetWorld()&&GetWorld()->GetTimeSeconds()<TemporaryAnimationUntil))return;
	if(bLocomotionMoving)
	{
		if(SmoothedLocomotionSpeed<StopWalkingSpeed)bLocomotionMoving=false;
	}
	else if(SmoothedLocomotionSpeed>StartWalkingSpeed)bLocomotionMoving=true;
	const float WalkRate=FMath::Clamp(SmoothedLocomotionSpeed/(IsCrawling()?CrawlSpeed:270.f)*WalkAnimationPlayRate,.72f,2.3f);
	if(IsCrawling())PlayZombieAnimation(CrawlAnimation?CrawlAnimation:WalkAnimation,true,WalkRate);
	else
	{
		UAnimSequence* MovingAnimation=(RunAnimation&&GetCharacterMovement()->MaxWalkSpeed>=400.f)?RunAnimation:WalkAnimation;
		PlayZombieAnimation(bLocomotionMoving?MovingAnimation:IdleAnimation,true,bLocomotionMoving?WalkRate:1.f);
	}
}

void AZombieCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AZombieCharacter,HeadHits);
	DOREPLIFETIME(AZombieCharacter,TorsoHits);
	DOREPLIFETIME(AZombieCharacter,LimbHits);
	DOREPLIFETIME(AZombieCharacter,LethalProgress);
	DOREPLIFETIME(AZombieCharacter,CombatHealth);
	DOREPLIFETIME(AZombieCharacter,bIsDead);
	DOREPLIFETIME(AZombieCharacter,AttackMode);
	DOREPLIFETIME(AZombieCharacter,bDormant);
	DOREPLIFETIME(AZombieCharacter,SeveredLimbs);
}

bool AZombieCharacter::IsHeadBone(FName BoneName)
{
	return BoneName.ToString().ToLower().Contains(TEXT("head"));
}

bool AZombieCharacter::IsLimbBone(FName BoneName)
{
	const FString Bone=BoneName.ToString().ToLower();
	static const TCHAR* LimbTokens[]={TEXT("arm"),TEXT("hand"),TEXT("finger"),TEXT("thumb"),TEXT("leg"),TEXT("upleg"),TEXT("foot"),TEXT("toe"),TEXT("thigh"),TEXT("calf")};
	for(const TCHAR* Token:LimbTokens)if(Bone.Contains(Token))return true;
	return false;
}

int32 AZombieCharacter::ResolveLimbIndex(FName BoneName)const
{
	const FString Bone=BoneName.ToString().ToLower();
	const bool bArm=Bone.Contains(TEXT("arm"))||Bone.Contains(TEXT("hand"))||Bone.Contains(TEXT("finger"))||Bone.Contains(TEXT("thumb"));
	const bool bLeg=Bone.Contains(TEXT("leg"))||Bone.Contains(TEXT("upleg"))||Bone.Contains(TEXT("thigh"))||Bone.Contains(TEXT("calf"))||Bone.Contains(TEXT("foot"))||Bone.Contains(TEXT("toe"));
	if(!bArm&&!bLeg)return INDEX_NONE;
	const bool bLeft=Bone.Contains(TEXT("left"))||Bone.Contains(TEXT("_l"))||Bone.EndsWith(TEXT("l"));
	const bool bRight=Bone.Contains(TEXT("right"))||Bone.Contains(TEXT("_r"))||Bone.EndsWith(TEXT("r"));
	if(!bLeft&&!bRight)return INDEX_NONE;
	return bLeg?(bLeft?2:3):(bLeft?0:1);
}

FName AZombieCharacter::ResolveLimbRootBone(int32 LimbIndex)const
{
	if(!GetMesh()||LimbIndex<0||LimbIndex>3)return NAME_None;
	const bool bLeft=(LimbIndex==0||LimbIndex==2);const bool bLeg=LimbIndex>=2;
	FName Fallback=NAME_None;
	for(int32 BoneIndex=0;BoneIndex<GetMesh()->GetNumBones();++BoneIndex)
	{
		const FName Candidate=GetMesh()->GetBoneName(BoneIndex);const FString Bone=Candidate.ToString().ToLower();
		const bool bCorrectSide=bLeft?(Bone.Contains(TEXT("left"))||Bone.Contains(TEXT("_l"))||Bone.EndsWith(TEXT("l"))):(Bone.Contains(TEXT("right"))||Bone.Contains(TEXT("_r"))||Bone.EndsWith(TEXT("r")));
		if(!bCorrectSide)continue;
		if(bLeg&&(Bone.Contains(TEXT("upleg"))||Bone.Contains(TEXT("thigh"))))return Candidate;
		if(!bLeg&&Bone.Contains(TEXT("upperarm")))return Candidate;
		if(Fallback.IsNone()&&((bLeg&&IsLimbBone(Candidate))||(!bLeg&&(Bone.Contains(TEXT("arm"))||Bone.Contains(TEXT("hand"))))))Fallback=Candidate;
	}
	return Fallback;
}

void AZombieCharacter::SeverLimb(int32 LimbIndex,FName HitBone,const FVector& ShotDirection,const FVector& HitLocation)
{
	if(LimbIndex<0||LimbIndex>3)return;
	const uint8 Bit=static_cast<uint8>(1<<LimbIndex);if((SeveredLimbs&Bit)!=0)return;
	SeveredLimbs|=Bit;
	const FName RootBone=ResolveLimbRootBone(LimbIndex);
	const FVector LimbLocation=RootBone.IsNone()?HitLocation:GetMesh()->GetBoneLocation(RootBone);
	const FVector Impulse=ShotDirection.GetSafeNormal()*HeadDetachImpulse*.62f+FVector(0.f,0.f,4200.f);
	MulticastSeverLimb(Bit,RootBone,Impulse,LimbLocation);
	ApplySeveredLimbState();
}

void AZombieCharacter::ApplySeveredLimbState()
{
	if(!GetMesh())return;
	for(int32 LimbIndex=0;LimbIndex<4;++LimbIndex)if((SeveredLimbs&(1<<LimbIndex))!=0)
	{
		const FName Root=ResolveLimbRootBone(LimbIndex);if(!Root.IsNone())GetMesh()->HideBoneByName(Root,PBO_Term);
	}
	if(UCharacterMovementComponent* Movement=GetCharacterMovement())Movement->MaxWalkSpeed=(IsCrawling()?CrawlSpeed:BaseWalkSpeed)*(bNightEmpowered?1.55f:1.f);
	CurrentAnimation=nullptr;
	UpdateLocomotionAnimation();
}

void AZombieCharacter::OnRep_SeveredLimbs(){ApplySeveredLimbState();}

void AZombieCharacter::MulticastSeverLimb_Implementation(uint8 LimbBit,FName RootBone,FVector Impulse,FVector HitLocation)
{
	if(!RootBone.IsNone())GetMesh()->HideBoneByName(RootBone,PBO_Term);
	FActorSpawnParameters Parameters;Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if(ALimbGibActor* Gib=GetWorld()->SpawnActor<ALimbGibActor>(ALimbGibActor::StaticClass(),HitLocation,FRotator::ZeroRotator,Parameters))
		Gib->InitializeGib(GetMesh()->GetMaterial(0),(LimbBit&12)!=0,Impulse);
	if(ABloodBurstActor* Blood=GetWorld()->SpawnActor<ABloodBurstActor>(ABloodBurstActor::StaticClass(),HitLocation,Impulse.Rotation(),Parameters))
	{
		if(!RootBone.IsNone())Blood->AttachToComponent(GetMesh(),FAttachmentTransformRules::KeepWorldTransform,RootBone);
		Blood->ActivateBurst(Impulse.GetSafeNormal(),true);
	}
}

FName AZombieCharacter::ResolveHeadBone(FName PreferredBone)const
{
	if(IsHeadBone(PreferredBone))return PreferredBone;
	if(!GetMesh())return NAME_None;
	for(int32 BoneIndex=0;BoneIndex<GetMesh()->GetNumBones();++BoneIndex)
	{
		const FName Candidate=GetMesh()->GetBoneName(BoneIndex);
		if(IsHeadBone(Candidate))return Candidate;
	}
	return NAME_None;
}

float AZombieCharacter::TakeDamage(float DamageAmount,const FDamageEvent& DamageEvent,AController* EventInstigator,AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount,DamageEvent,EventInstigator,DamageCauser);
	if(!HasAuthority()||bIsDead)return 0.f;

	FName HitBone=NAME_None;
	FVector ShotDirection=GetActorForwardVector();
	FVector HitLocation=GetActorLocation();
	if(DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointEvent=static_cast<const FPointDamageEvent&>(DamageEvent);
		HitBone=PointEvent.HitInfo.BoneName;
		ShotDirection=PointEvent.ShotDirection.GetSafeNormal();
		HitLocation=PointEvent.HitInfo.ImpactPoint;
	}

	const bool bHeadshot=IsHeadBone(HitBone);
	int32 DismembermentHitPower=1;
	if(const AWeaponBase* Weapon=Cast<AWeaponBase>(DamageCauser))if(Weapon->bMeleeWeapon)DismembermentHitPower=FMath::Max(1,Weapon->MeleeDismembermentHitPower);
	if(bHeadshot)HeadHits+=DismembermentHitPower;
	else if(IsLimbBone(HitBone))
	{
		LimbHits+=DismembermentHitPower;
		const int32 LimbIndex=ResolveLimbIndex(HitBone);
		if(LimbIndex!=INDEX_NONE&&(SeveredLimbs&(1<<LimbIndex))==0)
		{
			const int32 RequiredHits=LimbIndex>=2?LegDetachHits:ArmDetachHits;
			LimbZoneHits[LimbIndex]+=DismembermentHitPower;
			if(LimbZoneHits[LimbIndex]>=RequiredHits)SeverLimb(LimbIndex,HitBone,ShotDirection,HitLocation);
		}
	}
	else ++TorsoHits;
	const float OldCombatHealth=CombatHealth;
	CombatHealth=FMath::Clamp(CombatHealth-FMath::Max(0.f,DamageAmount),0.f,MaxCombatHealth);
	const float AppliedDamage=OldCombatHealth-CombatHealth;
	LethalProgress=1.f-CombatHealth/FMath::Max(1.f,MaxCombatHealth);
	const bool bKilled=CombatHealth<=KINDA_SMALL_NUMBER;
	MulticastBloodImpact(HitLocation,ShotDirection,false);
	if(!bKilled&&!IsAttacking())BeginHitReaction();
	WakeUp();

	if(AZombieAIController* ZombieController=Cast<AZombieAIController>(GetController()))
	{
		AActor* Attacker=EventInstigator?EventInstigator->GetPawn():DamageCauser;
		ZombieController->AlertToActor(Attacker);
	}
	if(bKilled)
	{
		if(AShooterCharacter* Killer=EventInstigator?Cast<AShooterCharacter>(EventInstigator->GetPawn()):nullptr)
		{
			const int32 Reward=KillExperience+(bHeadshot?HeadshotBonusExperience:0);
			Killer->AddExperience(Reward);
			UE_LOG(LogTemp,Display,TEXT("Player %s earned %d XP for %s kill"),*Killer->GetName(),Reward,bHeadshot?TEXT("headshot"):TEXT("zombie"));
		}
		Die(bHeadshot,HitBone,ShotDirection,HitLocation);
	}
	UE_LOG(LogTemp,Verbose,TEXT("Zombie %s took %.1f zone damage to %s (%.1f / %.1f remaining)"),
		*GetName(),AppliedDamage,*HitBone.ToString(),CombatHealth,MaxCombatHealth);
	return AppliedDamage;
}

void AZombieCharacter::MulticastBloodImpact_Implementation(FVector_NetQuantize HitLocation,FVector_NetQuantizeNormal ShotDirection,bool bFountain)
{
	if(!GetWorld())return;
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if(ABloodBurstActor* Blood=GetWorld()->SpawnActor<ABloodBurstActor>(ABloodBurstActor::StaticClass(),HitLocation,ShotDirection.Rotation(),Parameters))
		Blood->ActivateBurst(ShotDirection,bFountain);
}

void AZombieCharacter::MulticastHitReaction_Implementation()
{
	if(!HitReactionAnimation||bIsDead||IsAttacking())return;
	TemporaryAnimationUntil=GetWorld()->GetTimeSeconds()+HitReactionAnimation->GetPlayLength()/1.25f;
	CurrentAnimation=nullptr;
	PlayZombieAnimation(HitReactionAnimation,false,1.25f);
}

void AZombieCharacter::BeginHitReaction()
{
	if(!HasAuthority()||bIsDead||IsAttacking()||!HitReactionAnimation)return;
	bHitReacting=true;
	if(AZombieAIController* ZombieController=Cast<AZombieAIController>(GetController()))ZombieController->StopMovement();
	if(UCharacterMovementComponent* Movement=GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	MulticastHitReaction();
	const float Duration=HitReactionAnimation->GetPlayLength()/1.25f;
	GetWorldTimerManager().SetTimer(HitReactionTimer,this,&AZombieCharacter::FinishHitReaction,FMath::Max(.1f,Duration),false);
}

void AZombieCharacter::FinishHitReaction()
{
	if(!HasAuthority())return;
	bHitReacting=false;
	if(!bIsDead&&!bDormant&&!bWakeSequencePlaying&&!IsAttacking())
	{
		if(UCharacterMovementComponent* Movement=GetCharacterMovement())Movement->SetMovementMode(MOVE_Walking);
		CurrentAnimation=nullptr;
		UpdateLocomotionAnimation();
	}
}

bool AZombieCharacter::TryAttack(AActor* Target)
{
	if(!HasAuthority()||bIsDead||IsAttacking()||bHitReacting||!Target||(SeveredLimbs&3)==3)return false;
	FVector AttackPoint=Target->GetActorLocation();
	if(Cast<ABuildableStructure>(Target))
	{
		const FBox Bounds=Target->GetComponentsBoundingBox();const FVector Here=GetActorLocation();
		AttackPoint=FVector(FMath::Clamp(Here.X,Bounds.Min.X,Bounds.Max.X),FMath::Clamp(Here.Y,Bounds.Min.Y,Bounds.Max.Y),FMath::Clamp(Here.Z,Bounds.Min.Z,Bounds.Max.Z));
	}
	if(FVector::DistSquared(AttackPoint,GetActorLocation())>FMath::Square(AttackRange))return false;
	const double Now=GetWorld()->GetTimeSeconds();
	if(Now-LastAttackTime<AttackCooldown)return false;

	LastAttackTime=Now;
	PendingAttackTarget=Target;
	ACharacter* CharacterTarget=Cast<ACharacter>(Target);
	const float TargetDistance2D=FVector::Dist2D(GetActorLocation(),Target->GetActorLocation());
	const bool bCanBite=CharacterTarget&&!Cast<ABuildableStructure>(Target)&&(SeveredLimbs&3)==0;
	const bool bStationaryPointBlank=bCanBite&&TargetDistance2D<=ImmediateBiteRange
		&&CharacterTarget->GetVelocity().Size2D()<=StationaryBiteSpeed;
	AttackMode=bCanBite&&(bStationaryPointBlank||FMath::FRand()<BiteChance)
		?EZombieAttackMode::Bite:EZombieAttackMode::HandStrike;
	GetCharacterMovement()->StopMovementImmediately();
	OnRep_AttackMode();
	ForceNetUpdate();
	const float SelectedHitDelay=AttackMode==EZombieAttackMode::Bite?BiteHitDelay:AttackHitDelay;
	GetWorldTimerManager().SetTimer(AttackHitTimer,this,&AZombieCharacter::PerformAttackHit,SelectedHitDelay,false);
	// A dedicated bite sequence can be assigned on a Blueprint child.  Until one
	// is available, keep the grapple functional and use the existing close attack
	// as a visual fallback instead of silently disabling bites.
	UAnimSequence* SelectedAttack=AttackMode==EZombieAttackMode::Bite&&BiteAnimation?BiteAnimation:AttackAnimation;
	const float SelectedPlayRate=AttackMode==EZombieAttackMode::Bite?BitePlayRate:AttackPlayRate;
	const float AnimationDuration=SelectedAttack?SelectedAttack->GetPlayLength()/FMath::Max(.1f,SelectedPlayRate):1.8f;
	GetWorldTimerManager().SetTimer(AttackFinishTimer,this,&AZombieCharacter::FinishAttack,FMath::Max(SelectedHitDelay+.2f,AnimationDuration),false);
	return true;
}

void AZombieCharacter::OnRep_AttackMode()
{
	CurrentAnimation=nullptr;
	if(AttackMode!=EZombieAttackMode::None)
	{
		UAnimSequence* SelectedAttack=AttackMode==EZombieAttackMode::Bite&&BiteAnimation?BiteAnimation:AttackAnimation;
		PlayZombieAnimation(SelectedAttack,false,AttackMode==EZombieAttackMode::Bite?BitePlayRate:AttackPlayRate);
	}
	else UpdateLocomotionAnimation();
}

void AZombieCharacter::PerformAttackHit()
{
	if(!HasAuthority()||bIsDead||!IsAttacking()||!PendingAttackTarget.IsValid())return;
	AActor* Target=PendingAttackTarget.Get();
	FVector TargetPoint=Target->GetActorLocation();
	if(Cast<ABuildableStructure>(Target))
	{
		const FBox Bounds=Target->GetComponentsBoundingBox();const FVector Here=GetActorLocation();
		TargetPoint=FVector(FMath::Clamp(Here.X,Bounds.Min.X,Bounds.Max.X),FMath::Clamp(Here.Y,Bounds.Min.Y,Bounds.Max.Y),FMath::Clamp(Here.Z,Bounds.Min.Z,Bounds.Max.Z));
	}
	FVector ToTarget=TargetPoint-GetActorLocation();
	const float Distance2D=ToTarget.Size2D();
	ToTarget.Z=0.f;
	if(Distance2D>AttackRange+35.f||ToTarget.IsNearlyZero())return;
	const FVector AttackDirection=ToTarget.GetSafeNormal();
	if(FVector::DotProduct(GetActorForwardVector(),AttackDirection)<AttackFacingThreshold)return;

	FHitResult ObstacleHit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ZombieAttack),false,this);
	const FVector TraceStart=GetActorLocation()+FVector(0.f,0.f,45.f);
	const FVector TraceEnd=TargetPoint+FVector(0.f,0.f,35.f);
	if(GetWorld()->LineTraceSingleByChannel(ObstacleHit,TraceStart,TraceEnd,ECC_Visibility,Query))
	{
		AActor* HitActor=ObstacleHit.GetActor();
		if(HitActor&&HitActor!=Target&&!HitActor->IsOwnedBy(Target))return;
	}
	if(AttackMode==EZombieAttackMode::Bite&&Cast<ACharacter>(Target))
	{
		BeginBite(Target);
		return;
	}

	if(UHealthArmorComponent* TargetHealth=Target->FindComponentByClass<UHealthArmorComponent>())
	{
		const float ArmMultiplier=(SeveredLimbs&3)!=0?.55f:1.f;
		const float AppliedDamage=TargetHealth->ApplyDamage(AttackDamage*ArmMultiplier,GetController(),this);
		if(AppliedDamage>0.f)UE_LOG(LogTemp,Display,TEXT("Zombie %s melee hit %s: %.0f damage, %.0f health remaining"),*GetName(),*GetNameSafe(Target),AppliedDamage,TargetHealth->Health);
		if(ACharacter* HitCharacter=Cast<ACharacter>(Target))HitCharacter->LaunchCharacter(AttackDirection*90.f+FVector(0.f,0.f,35.f),false,false);
	}
	else if(ABuildableStructure* Structure=Cast<ABuildableStructure>(Target))
	{
		Structure->TakeDamage(FMath::Max(20.f,AttackDamage*1.35f),FDamageEvent(),GetController(),this);
	}
	else
	{
		// Bandits keep their replicated health directly on the actor rather than in
		// UHealthArmorComponent, so route melee through the standard damage API.
		Target->TakeDamage(AttackDamage,FDamageEvent(),GetController(),this);
	}
}

void AZombieCharacter::FinishAttack()
{
	if(!HasAuthority()||bIsDead)return;
	EndBite();
	PendingAttackTarget.Reset();
	AttackMode=EZombieAttackMode::None;
	OnRep_AttackMode();
	ForceNetUpdate();
}

void AZombieCharacter::BeginBite(AActor* Victim)
{
	if(!HasAuthority()||!Victim||bIsDead)return;
	// A normal swipe never reaches this function. The grapple begins only after
	// the bite roll has selected the bite state and the timed melee trace has
	// confirmed that the victim is still in front of the zombie.
	if(AShooterCharacter* Player=Cast<AShooterCharacter>(Victim))
	{
		if(!Player->BeginZombieBite(this))
		{
			AttackMode=EZombieAttackMode::HandStrike;
			OnRep_AttackMode();
			ForceNetUpdate();
			return;
		}
	}
	BiteVictim=Victim;
	BiteStartTime=GetWorld()->GetTimeSeconds();
	BiteEscapePresses=0;
	if(ACharacter* Character=Cast<ACharacter>(Victim))
	{
		if(!Cast<AShooterCharacter>(Character))Character->GetCharacterMovement()->DisableMovement();
		const FVector ToVictim=Character->GetActorLocation()-GetActorLocation();
		if(!ToVictim.IsNearlyZero())SetActorRotation(FRotator(0.f,ToVictim.Rotation().Yaw,0.f));
	}
	GetWorldTimerManager().ClearTimer(AttackFinishTimer);
	ApplyBiteDamage();
	GetWorldTimerManager().SetTimer(BiteDamageTimer,this,&AZombieCharacter::ApplyBiteDamage,BiteDamageInterval,true);
	GetWorldTimerManager().SetTimer(AttackFinishTimer,this,&AZombieCharacter::FinishAttack,BiteMaximumDuration,false);
}

void AZombieCharacter::ApplyBiteDamage()
{
	if(!HasAuthority()||bIsDead||!BiteVictim.IsValid()){FinishAttack();return;}
	AActor* Victim=BiteVictim.Get();
	if(FVector::DistSquared2D(GetActorLocation(),Victim->GetActorLocation())>FMath::Square(AttackRange+80.f)){FinishAttack();return;}
	const float TickDamage=AttackDamage*.22f;
	if(UHealthArmorComponent* TargetHealth=Victim->FindComponentByClass<UHealthArmorComponent>())TargetHealth->ApplyDamage(TickDamage,GetController(),this);
	else Victim->TakeDamage(TickDamage,FDamageEvent(),GetController(),this);
}

void AZombieCharacter::RegisterBiteEscapePress(AActor* Victim)
{
	if(!HasAuthority()||Victim!=BiteVictim.Get()||AttackMode!=EZombieAttackMode::Bite)return;
	++BiteEscapePresses;
	if(BiteEscapePresses<BiteEscapePressesRequired)return;
	const float Elapsed=GetWorld()->GetTimeSeconds()-BiteStartTime;
	if(Elapsed>=BiteMinimumDuration)FinishAttack();
	else GetWorldTimerManager().SetTimer(AttackFinishTimer,this,&AZombieCharacter::FinishAttack,BiteMinimumDuration-Elapsed,false);
}

void AZombieCharacter::EndBite()
{
	GetWorldTimerManager().ClearTimer(BiteDamageTimer);
	if(!BiteVictim.IsValid())return;
	if(AShooterCharacter* Player=Cast<AShooterCharacter>(BiteVictim.Get()))Player->EndZombieBite(this);
	else if(ACharacter* Character=Cast<ACharacter>(BiteVictim.Get()))
	{
		const ABanditCharacter* Bandit=Cast<ABanditCharacter>(Character);
		if(!Bandit||!Bandit->IsDead())Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	BiteVictim.Reset();
}

void AZombieCharacter::Die(bool bHeadshot,FName HitBone,const FVector& ShotDirection,const FVector& HitLocation)
{
	if(bIsDead)return;
	bIsDead=true;
	EndBite();
	AttackMode=EZombieAttackMode::None;
	bHitReacting=false;
	PendingAttackTarget.Reset();
	GetWorldTimerManager().ClearTimer(AttackHitTimer);
	GetWorldTimerManager().ClearTimer(AttackFinishTimer);
	GetWorldTimerManager().ClearTimer(HitReactionTimer);
	const FVector Impulse=ShotDirection.GetSafeNormal()*HeadDetachImpulse;
	MulticastDie(bHeadshot,HitBone,Impulse,HitLocation);
	if(HasAuthority()&&GetWorld()&&FMath::FRand()<=LootBagDropChance)
	{
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FVector DropLocation=GetActorLocation()+FVector(0.f,0.f,45.f);
		GetWorld()->SpawnActor<ALootBagPickup>(ALootBagPickup::StaticClass(),DropLocation,FRotator::ZeroRotator,Parameters);
	}
	SetLifeSpan(20.f);
}

void AZombieCharacter::MulticastDie_Implementation(bool bHeadshot,FName HitBone,FVector Impulse,FVector HitLocation)
{
	bIsDead=true;
	AttackMode=EZombieAttackMode::None;
	CurrentAnimation=nullptr;
	if(AAIController* ZombieController=Cast<AAIController>(GetController()))ZombieController->StopMovement();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
	if(GetWorld())
	{
		FActorSpawnParameters PoolParameters;
		PoolParameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if(ABloodPoolActor* Pool=GetWorld()->SpawnActor<ABloodPoolActor>(ABloodPoolActor::StaticClass(),GetActorLocation(),FRotator::ZeroRotator,PoolParameters))
			Pool->ActivatePool(GetMesh(),GetActorLocation());
	}
	if(bHeadshot)
	{
		const FName HeadBone=ResolveHeadBone(HitBone);
		const FVector HeadLocation=HeadBone.IsNone()?HitLocation:GetMesh()->GetBoneLocation(HeadBone);
		UMaterialInterface* HeadMaterial=GetMesh()->GetMaterial(0);
		if(!HeadBone.IsNone())GetMesh()->HideBoneByName(HeadBone,PBO_Term);
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if(AHeadGibActor* DetachedHead=GetWorld()->SpawnActor<AHeadGibActor>(AHeadGibActor::StaticClass(),HeadLocation,FRotator::ZeroRotator,Parameters))
			DetachedHead->InitializeGib(HeadMaterial,Impulse);
		if(ABloodBurstActor* Blood=GetWorld()->SpawnActor<ABloodBurstActor>(ABloodBurstActor::StaticClass(),HeadLocation,Impulse.Rotation(),Parameters))
		{
			if(!HeadBone.IsNone())Blood->AttachToComponent(GetMesh(),FAttachmentTransformRules::KeepWorldTransform,HeadBone);
			Blood->ActivateBurst(Impulse.GetSafeNormal(),true);
		}
	}
}
