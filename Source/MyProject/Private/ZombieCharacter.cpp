#include "ZombieCharacter.h"

#include "ZombieAIController.h"
#include "BloodBurstActor.h"
#include "HeadGibActor.h"
#include "LootBagPickup.h"
#include "BuildableStructure.h"
#include "ShooterCharacter.h"
#include "HealthArmorComponent.h"
#include "OpenWorldStreamingManager.h"
#include "Animation/AnimSequence.h"
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
}

void AZombieCharacter::BeginPlay()
{
	Super::BeginPlay();
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
	UpdateNightEmpowerment();
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
	if(GetWorld()&&GetWorld()->GetTimeSeconds()>=NextNightStateCheck){NextNightStateCheck=GetWorld()->GetTimeSeconds()+1.f;UpdateNightEmpowerment();}
	SmoothedLocomotionSpeed=FMath::FInterpTo(SmoothedLocomotionSpeed,GetVelocity().Size2D(),DeltaSeconds,LocomotionSmoothingSpeed);
	if(bIsAttacking)
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
	GetCharacterMovement()->MaxWalkSpeed=BaseWalkSpeed*(bEnable?1.55f:1.f);
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
	if(bIsDead||bIsAttacking)return;
	if(bLocomotionMoving)
	{
		if(SmoothedLocomotionSpeed<StopWalkingSpeed)bLocomotionMoving=false;
	}
	else if(SmoothedLocomotionSpeed>StartWalkingSpeed)bLocomotionMoving=true;
	const float WalkRate=FMath::Clamp(SmoothedLocomotionSpeed/270.f*WalkAnimationPlayRate,.72f,2.3f);
	PlayZombieAnimation(bLocomotionMoving?WalkAnimation:IdleAnimation,true,bLocomotionMoving?WalkRate:1.f);
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
	DOREPLIFETIME(AZombieCharacter,bIsAttacking);
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
	if(bHeadshot)++HeadHits;
	else if(IsLimbBone(HitBone))++LimbHits;
	else ++TorsoHits;
	const float OldCombatHealth=CombatHealth;
	CombatHealth=FMath::Clamp(CombatHealth-FMath::Max(0.f,DamageAmount),0.f,MaxCombatHealth);
	const float AppliedDamage=OldCombatHealth-CombatHealth;
	LethalProgress=1.f-CombatHealth/FMath::Max(1.f,MaxCombatHealth);
	const bool bKilled=CombatHealth<=KINDA_SMALL_NUMBER;
	MulticastBloodImpact(HitLocation,ShotDirection,bKilled&&bHeadshot);

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

bool AZombieCharacter::TryAttack(AActor* Target)
{
	if(!HasAuthority()||bIsDead||bIsAttacking||!Target)return false;
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
	bIsAttacking=true;
	GetCharacterMovement()->StopMovementImmediately();
	OnRep_IsAttacking();
	GetWorldTimerManager().SetTimer(AttackHitTimer,this,&AZombieCharacter::PerformAttackHit,AttackHitDelay,false);
	const float AnimationDuration=AttackAnimation?AttackAnimation->GetPlayLength()/FMath::Max(.1f,AttackPlayRate):1.8f;
	GetWorldTimerManager().SetTimer(AttackFinishTimer,this,&AZombieCharacter::FinishAttack,FMath::Max(AttackHitDelay+.2f,AnimationDuration),false);
	return true;
}

void AZombieCharacter::OnRep_IsAttacking()
{
	CurrentAnimation=nullptr;
	if(bIsAttacking)PlayZombieAnimation(AttackAnimation,false,AttackPlayRate);
	else UpdateLocomotionAnimation();
}

void AZombieCharacter::PerformAttackHit()
{
	if(!HasAuthority()||bIsDead||!bIsAttacking||!PendingAttackTarget.IsValid())return;
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

	if(UHealthArmorComponent* TargetHealth=Target->FindComponentByClass<UHealthArmorComponent>())
	{
		const float AppliedDamage=TargetHealth->ApplyDamage(AttackDamage,GetController(),this);
		if(AppliedDamage>0.f)UE_LOG(LogTemp,Display,TEXT("Zombie %s melee hit %s: %.0f damage, %.0f health remaining"),*GetName(),*GetNameSafe(Target),AppliedDamage,TargetHealth->Health);
		if(ACharacter* HitCharacter=Cast<ACharacter>(Target))HitCharacter->LaunchCharacter(AttackDirection*90.f+FVector(0.f,0.f,35.f),false,false);
	}
	else if(ABuildableStructure* Structure=Cast<ABuildableStructure>(Target))
	{
		Structure->TakeDamage(FMath::Max(20.f,AttackDamage*1.35f),FDamageEvent(),GetController(),this);
	}
}

void AZombieCharacter::FinishAttack()
{
	if(!HasAuthority()||bIsDead)return;
	PendingAttackTarget.Reset();
	bIsAttacking=false;
	OnRep_IsAttacking();
}

void AZombieCharacter::Die(bool bHeadshot,FName HitBone,const FVector& ShotDirection,const FVector& HitLocation)
{
	if(bIsDead)return;
	bIsDead=true;
	bIsAttacking=false;
	PendingAttackTarget.Reset();
	GetWorldTimerManager().ClearTimer(AttackHitTimer);
	GetWorldTimerManager().ClearTimer(AttackFinishTimer);
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
	bIsAttacking=false;
	CurrentAnimation=nullptr;
	if(AAIController* ZombieController=Cast<AAIController>(GetController()))ZombieController->StopMovement();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
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
	}
}
