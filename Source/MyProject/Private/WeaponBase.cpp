#include "WeaponBase.h"
#include "BallisticProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "BuildableStructure.h"
#include "ShooterCharacter.h"
#include "ZombieCharacter.h"
#include "Camera/CameraComponent.h"
#include "ZombieAIController.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AWeaponBase::AWeaponBase(){PrimaryActorTick.bCanEverTick=true;bReplicates=true;SetReplicateMovement(true);Mesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WorldWeaponMesh"));RootComponent=Mesh;Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetOwnerNoSee(false);FirstPersonMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonWeaponMesh"));FirstPersonMesh->SetupAttachment(RootComponent);FirstPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);FirstPersonMesh->SetVisibility(false,true);FirstPersonMesh->SetHiddenInGame(true);FirstPersonMesh->CastShadow=false;ProjectileClass=ABallisticProjectile::StaticClass();static ConstructorHelpers::FObjectFinder<UAnimMontage>HipFire(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Montages/Fire_Rifle_Hip_Montage.Fire_Rifle_Hip_Montage"));if(HipFire.Succeeded())CharacterFireMontage=HipFire.Object;static ConstructorHelpers::FObjectFinder<UAnimMontage>AimFire(TEXT("/Game/ThirdPersonBP/Player_0/Anim/Montages/Fire_Rifle_ironsights_Montage.Fire_Rifle_ironsights_Montage"));if(AimFire.Succeeded())CharacterAimFireMontage=AimFire.Object;AmmoInMagazine=Stats.MagazineSize;ReserveAmmo=90;}
void AWeaponBase::Tick(float D){Super::Tick(D);}
FVector AWeaponBase::GetMuzzleLocation()const{static const FName MuzzleBone(TEXT("b_gun_muzzleflash"));return Mesh->DoesSocketExist(MuzzleBone)?Mesh->GetSocketLocation(MuzzleBone):Mesh->GetComponentLocation()+GetActorForwardVector()*100.f;}
bool AWeaponBase::CanFire()const{return !bIsReloading&&GetWorld()&&GetWorld()->GetTimeSeconds()-LastFireTime>=60./FMath::Max(1.f,Stats.RoundsPerMinute)&&(bMeleeWeapon||(AmmoInMagazine>0&&ProjectileClass!=nullptr));}
bool AWeaponBase::IsMeleeActionAnimationPlaying()const{return GetWorld()&&GetWorld()->GetTimeSeconds()<MeleeActionAnimationUntil;}
bool AWeaponBase::Fire(FVector AimPoint){if(!HasAuthority()){if(!CanFire())return false;ServerFire(AimPoint);return true;}return FireAuthoritative(AimPoint);}
bool AWeaponBase::ServerFire_Validate(FVector_NetQuantize AimPoint){return !AimPoint.ContainsNaN();}void AWeaponBase::ServerFire_Implementation(FVector_NetQuantize AimPoint){FireAuthoritative(AimPoint);}
bool AWeaponBase::FireAuthoritative(FVector AimPoint)
{
	if(!CanFire())return false;
	LastFireTime=GetWorld()->GetTimeSeconds();
	if(bMeleeWeapon)
	{
		AShooterCharacter* Shooter=Cast<AShooterCharacter>(GetOwner());
		const FVector Start=Shooter&&Shooter->Camera?Shooter->Camera->GetComponentLocation():GetActorLocation();
		FVector Direction=Shooter&&Shooter->Camera?Shooter->Camera->GetForwardVector():GetActorForwardVector();
		if(!AimPoint.IsNearlyZero())Direction=(AimPoint-Start).GetSafeNormal();
		FCollisionQueryParams Query(SCENE_QUERY_STAT(MeleeWeaponSweep),true,GetOwner());Query.AddIgnoredActor(this);
		FHitResult Hit;
		bool bHit=GetWorld()->SweepSingleByChannel(Hit,Start,Start+Direction*MeleeRange,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(MeleeRadius),Query);
		// Character capsules are not required to block Visibility in this project.
		// Query Pawns explicitly so a visible axe swing cannot pass through a zombie
		// without producing an authoritative damage hit.
		FCollisionObjectQueryParams PawnObjects;
		PawnObjects.AddObjectTypesToQuery(ECC_Pawn);
		FHitResult PawnHit;
		if(GetWorld()->SweepSingleByObjectType(PawnHit,Start,Start+Direction*MeleeRange,FQuat::Identity,PawnObjects,FCollisionShape::MakeSphere(MeleeRadius),Query)
			&&PawnHit.GetActor()&&(!bHit||PawnHit.Distance<=Hit.Distance+MeleeRadius))
		{
			Hit=PawnHit;
			bHit=true;
		}
		if(bHit&&Hit.GetActor())
		{
			// A world sweep normally contacts the character capsule first.  Recover a
			// skeletal hit (or the nearest actual bone) so axe strikes preserve zones
			// for head and limb dismemberment instead of always becoming torso damage.
			if(AZombieCharacter* Zombie=Cast<AZombieCharacter>(Hit.GetActor()))
			{
				if(USkeletalMeshComponent* ZombieMesh=Zombie->GetMesh())
				{
					FHitResult SkeletalHit;
					const FVector TraceEnd=Start+Direction*(MeleeRange+MeleeRadius+40.f);
					if(ZombieMesh->LineTraceComponent(SkeletalHit,Start,TraceEnd,Query)&&!SkeletalHit.BoneName.IsNone())Hit=SkeletalHit;
					if(Hit.BoneName.IsNone())
					{
						FName ClosestBone=NAME_None;FVector ClosestLocation=Hit.ImpactPoint;float ClosestDistanceSquared=MAX_flt;
						for(int32 BoneIndex=0;BoneIndex<ZombieMesh->GetNumBones();++BoneIndex)
						{
							const FName Candidate=ZombieMesh->GetBoneName(BoneIndex);
							const FVector CandidateLocation=ZombieMesh->GetBoneLocation(Candidate);
							const float DistanceSquared=FVector::DistSquared(CandidateLocation,Hit.ImpactPoint);
							if(DistanceSquared<ClosestDistanceSquared){ClosestDistanceSquared=DistanceSquared;ClosestBone=Candidate;ClosestLocation=CandidateLocation;}
						}
						if(!ClosestBone.IsNone()&&ClosestDistanceSquared<=FMath::Square(90.f)){Hit.BoneName=ClosestBone;Hit.ImpactPoint=ClosestLocation;Hit.Location=ClosestLocation;}
					}
				}
			}
			const float Damage=Hit.GetActor()->IsA<ABuildableStructure>()?MeleeDamage*WoodDamageMultiplier:MeleeDamage;
			UGameplayStatics::ApplyPointDamage(Hit.GetActor(),Damage,Direction,Hit,GetOwner()?GetOwner()->GetInstigatorController():nullptr,this,nullptr);
		}
		const int32 AttackCount=CharacterMeleeAttackAnimations.Num();
		int32 AttackIndex=AttackCount>0?FMath::RandHelper(AttackCount):INDEX_NONE;
		// Preserve variety: when several attacks exist, never repeat the immediately
		// preceding animation. The authoritative server chooses once for all clients.
		if(AttackCount>1&&AttackIndex==LastMeleeAttackIndex)
			AttackIndex=(AttackIndex+1+FMath::RandHelper(AttackCount-1))%AttackCount;
		LastMeleeAttackIndex=AttackIndex;
		UE_LOG(LogTemp,Display,TEXT("Melee attack requested: weapon=%s attacks=%d index=%d authority=%d"),
			*GetNameSafe(this),AttackCount,AttackIndex,HasAuthority()?1:0);
		MulticastMeleeEffects(AttackIndex);
		return true;
	}
	--AmmoInMagazine;
	const FVector Muzzle=GetMuzzleLocation();
	if(AimPoint.IsNearlyZero())AimPoint=Muzzle+GetActorForwardVector()*100000.f;
	const FVector Shot=FMath::VRandCone((AimPoint-Muzzle).GetSafeNormal(),FMath::DegreesToRadians(Stats.SpreadDegrees));
	FActorSpawnParameters P;P.Owner=GetOwner();P.Instigator=Cast<APawn>(GetOwner());
	ABallisticProjectile* B=GetWorld()->SpawnActor<ABallisticProjectile>(ProjectileClass,Muzzle,Shot.Rotation(),P);
	if(B)
	{
		B->InitializeProjectile(Stats.Damage,Stats.HeadshotDamageMultiplier,Stats.LimbDamageMultiplier,Stats.DragCoefficient,Stats.WindInfluence,Stats.GravityScale,
			Stats.ProjectileLifeSeconds,GetOwner()?GetOwner()->GetInstigatorController():nullptr);
		if(CloseRangeHitCorrectionDistance>0.f&&FVector::DistSquared(Muzzle,AimPoint)<=FMath::Square(CloseRangeHitCorrectionDistance))
		{
			FHitResult IntendedHit;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(CloseRangeAimCorrection),true,GetOwner());
			Query.AddIgnoredActor(this);
			FVector ViewStart=Muzzle;
			if(const AShooterCharacter* Shooter=Cast<AShooterCharacter>(GetOwner()))
				if(Shooter->Camera)ViewStart=Shooter->Camera->GetComponentLocation();
			const FVector ViewDirection=(AimPoint-ViewStart).GetSafeNormal();
			if(GetWorld()->LineTraceSingleByChannel(IntendedHit,ViewStart,AimPoint+ViewDirection*12.f,ECC_Visibility,Query))
			{
				if(AZombieCharacter* IntendedZombie=Cast<AZombieCharacter>(IntendedHit.GetActor()))
				{
					USkeletalMeshComponent* ZombieMesh=IntendedZombie->GetMesh();
					const FVector BoneTraceEnd=AimPoint+ViewDirection*300.f;
					FHitResult SkeletalHit;
					if(ZombieMesh&&ZombieMesh->LineTraceComponent(SkeletalHit,ViewStart,BoneTraceEnd,Query)&&!SkeletalHit.BoneName.IsNone())
						IntendedHit=SkeletalHit;
					if(ZombieMesh&&(IntendedHit.BoneName.IsNone()||!IntendedHit.BoneName.ToString().ToLower().Contains(TEXT("head"))))
					{
						FName HeadBone=NAME_None;
						for(int32 BoneIndex=0;BoneIndex<ZombieMesh->GetNumBones();++BoneIndex)
						{
							const FName Candidate=ZombieMesh->GetBoneName(BoneIndex);
							if(Candidate.ToString().ToLower().Contains(TEXT("head"))){HeadBone=Candidate;break;}
						}
						if(!HeadBone.IsNone())
						{
							const FVector HeadLocation=ZombieMesh->GetBoneLocation(HeadBone);
							const FVector ClosestPoint=FMath::ClosestPointOnSegment(HeadLocation,ViewStart,BoneTraceEnd);
							if(FVector::DistSquared(ClosestPoint,HeadLocation)<=FMath::Square(32.f))
							{
								IntendedHit.BoneName=HeadBone;
								IntendedHit.ImpactPoint=HeadLocation;
								IntendedHit.Location=HeadLocation;
							}
						}
					}
				}
				B->SetCloseRangeHitCorrection(IntendedHit.GetActor(),IntendedHit.BoneName,IntendedHit.ImpactPoint);
			}
		}
		B->Movement->Velocity=Shot*Stats.MuzzleVelocity;
	}
	MulticastFireEffects(Muzzle,Shot.Rotation());
	// Gunfire is a major horde stimulus: 200 m keeps distant spawned zombies from
	// continuing their patrol while the player is actively firing at a base.
	const float HearingRadius=GunshotAlertRadius;
	UAISense_Hearing::ReportNoiseEvent(GetWorld(),Muzzle,Stats.NoiseLoudness,GetOwner(),HearingRadius,TEXT("Gunshot"));
	// Perception listeners can be registered one frame later than a dynamically
	// spawned controller in cooked builds. Notify the same server-side controllers
	// explicitly so a real gunshot is never lost during that initialization window.
	for(TActorIterator<AZombieAIController> It(GetWorld());It;++It)
	{
		AZombieAIController* ZombieController=*It;
		APawn* ZombiePawn=ZombieController?ZombieController->GetPawn():nullptr;
		if(ZombiePawn&&FVector::DistSquared(ZombiePawn->GetActorLocation(),Muzzle)<=FMath::Square(HearingRadius))
			ZombieController->AlertToActor(GetOwner());
	}
	return true;
}
void AWeaponBase::MulticastFireEffects_Implementation(FVector_NetQuantize Muzzle,FRotator Rotation)
{
	static const FName MuzzleBone(TEXT("b_gun_muzzleflash"));
	if(MuzzleFlash)
	{
		if(Mesh&&Mesh->DoesSocketExist(MuzzleBone))
			UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlash,Mesh,MuzzleBone,FVector::ZeroVector,
				FRotator::ZeroRotator,EAttachLocation::SnapToTarget,true,true,ENCPoolMethod::None,true);
		else UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),MuzzleFlash,Muzzle,Rotation);
	}
	if(FireSound)UGameplayStatics::PlaySoundAtLocation(this,FireSound,Muzzle);
	if(ACharacter*C=Cast<ACharacter>(GetOwner()))
		if(C->GetMesh()&&C->GetMesh()->GetAnimInstance())
		{
			UAnimMontage*Montage=CharacterFireMontage;
			if(const AShooterCharacter*S=Cast<AShooterCharacter>(C))
				if(S->IsAiming())
				{
					if(S->IsLocallyControlled()&&!bPlayCharacterAimFireMontage)Montage=nullptr;
					else if(CharacterAimFireMontage)Montage=CharacterAimFireMontage;
				}
			if(Montage)C->GetMesh()->GetAnimInstance()->Montage_Play(Montage);
		}
}
void AWeaponBase::MulticastMeleeEffects_Implementation(int32 AttackIndex)
{
	ACharacter* Character=Cast<ACharacter>(GetOwner());
	UAnimSequence* Attack=CharacterMeleeAttackAnimations.IsValidIndex(AttackIndex)?CharacterMeleeAttackAnimations[AttackIndex]:nullptr;
	if(!Character||!Character->GetMesh()||!Attack)
	{
		UE_LOG(LogTemp,Error,TEXT("Melee animation rejected: character=%s mesh=%s attackIndex=%d attack=%s count=%d"),
			*GetNameSafe(Character),Character?*GetNameSafe(Character->GetMesh()):TEXT("None"),AttackIndex,
			*GetNameSafe(Attack),CharacterMeleeAttackAnimations.Num());
		return;
	}
	if(UAnimInstance* AnimInstance=Character->GetMesh()->GetAnimInstance())
	{
		// UE4ASP_HeroTPP_AnimBlueprint routes melee actions through AxeSlot. A dynamic
		// montage preserves the complete AnimBP graph and only layers the authored
		// axe swing for its duration instead of replacing the player's AnimInstance.
		MeleeActionAnimationUntil=GetWorld()->GetTimeSeconds()+Attack->GetPlayLength();
		static const FName AxeSlotName(TEXT("AxeSlot"));
		// A slot node can be saved in an AnimBP without the corresponding slot being
		// persisted in the Skeleton asset. Dynamic montage creation then silently
		// returns null, so register it on the animation skeleton before playback.
		if(USkeleton* AttackSkeleton=Attack->GetSkeleton())AttackSkeleton->RegisterSlotNode(AxeSlotName);
		USkeleton* MeshSkeleton=Character->GetMesh()->SkeletalMesh?Character->GetMesh()->SkeletalMesh->Skeleton:nullptr;
		USkeleton* AttackSkeleton=Attack->GetSkeleton();
		const bool bCompatible=MeshSkeleton&&AttackSkeleton&&MeshSkeleton->IsCompatible(AttackSkeleton);
		const bool bComposable=Attack->CanBeUsedInComposition();
		UAnimMontage* DynamicMontage=UAnimMontage::CreateSlotAnimationAsDynamicMontage(Attack,AxeSlotName,.08f,.16f,1.f,1,.05f,.05f);
		const float PlayedLength=DynamicMontage?AnimInstance->Montage_Play(DynamicMontage,1.f,EMontagePlayReturnType::MontageLength,.05f):0.f;
		UE_LOG(LogTemp,Display,TEXT("Melee animation play: character=%s animInstance=%s attack=%s length=%.3f meshSkeleton=%s attackSkeleton=%s compatible=%d composable=%d slot=AxeSlot montage=%s played=%.3f"),
			*GetNameSafe(Character),*GetNameSafe(AnimInstance),*GetNameSafe(Attack),Attack->GetPlayLength(),
			*GetNameSafe(MeshSkeleton),*GetNameSafe(AttackSkeleton),bCompatible?1:0,bComposable?1:0,*GetNameSafe(DynamicMontage),PlayedLength);
	}
	else UE_LOG(LogTemp,Error,TEXT("Melee animation rejected: %s has no AnimInstance"),*GetNameSafe(Character));
}
void AWeaponBase::Reload(){if(HasAuthority())ServerReload_Implementation();else ServerReload();}bool AWeaponBase::ServerReload_Validate(){return true;}void AWeaponBase::ServerReload_Implementation(){if(bIsReloading||AmmoInMagazine>=Stats.MagazineSize||ReserveAmmo<=0)return;bIsReloading=true;const AShooterCharacter* Shooter=Cast<AShooterCharacter>(GetOwner());const float ReloadMultiplier=Shooter?Shooter->GetReloadTimeMultiplier():1.f;GetWorldTimerManager().SetTimer(ReloadTimer,this,&AWeaponBase::FinishReload,FMath::Max(.1f,Stats.ReloadSeconds*ReloadMultiplier),false);}
void AWeaponBase::FinishReload(){if(!HasAuthority())return;const int32 Need=Stats.MagazineSize-AmmoInMagazine,Take=FMath::Min(Need,ReserveAmmo);AmmoInMagazine+=Take;ReserveAmmo-=Take;bIsReloading=false;}
int32 AWeaponBase::AddReserveAmmo(int32 Amount){if(!HasAuthority()||Amount<=0)return 0;const int32 Accepted=FMath::Min(Amount,FMath::Max(0,MaxReserveAmmo-ReserveAmmo));ReserveAmmo+=Accepted;return Accepted;}
void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AWeaponBase,AmmoInMagazine);DOREPLIFETIME(AWeaponBase,ReserveAmmo);DOREPLIFETIME(AWeaponBase,bIsReloading);}
