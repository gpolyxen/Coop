#include "BanditCharacter.h"
#include "BanditAIController.h"
#include "BallisticProjectile.h"
#include "HealthArmorComponent.h"
#include "ShooterCharacter.h"
#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Perception/AISense_Hearing.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABanditCharacter::ABanditCharacter()
{
	PrimaryActorTick.bCanEverTick=false;bReplicates=true;SetReplicateMovement(true);
	AIControllerClass=ABanditAIController::StaticClass();AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw=false;GetCharacterMovement()->bOrientRotationToMovement=true;
	GetCharacterMovement()->RotationRate=FRotator(0.f,500.f,0.f);GetCharacterMovement()->MaxWalkSpeed=390.f;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch=true;GetCharacterMovement()->GetNavAgentPropertiesRef().bCanJump=true;
	GetCharacterMovement()->JumpZVelocity=520.f;
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mannequin(TEXT("/Game/ThirdPersonBP/Player_0/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin"));
	if(Mannequin.Succeeded()){GetMesh()->SetSkeletalMesh(Mannequin.Object);GetMesh()->SetRelativeLocation(FVector(0,0,-100));GetMesh()->SetRelativeRotation(FRotator(0,-90,0));}
	// Use the exact armed AnimBP used by AShooterCharacter.  The stock mannequin
	// AnimBP was not the graph that drives this project's rifle locomotion and left
	// spawned bandits in the reference pose in cooked builds.
	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/ThirdPersonBP/Player_0/Anim/UE4ASP_HeroTPP_AnimBlueprint"));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	if(Anim.Succeeded())GetMesh()->SetAnimInstanceClass(Anim.Class);
	WeaponMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BanditRifle"));WeaponMesh->SetupAttachment(GetMesh(),TEXT("sktGun"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Rifle(TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/AR4/SK_AR4_X.SK_AR4_X"));if(Rifle.Succeeded())WeaponMesh->SetSkeletalMesh(Rifle.Object);
	ProjectileClass=ABallisticProjectile::StaticClass();
}

void ABanditCharacter::BeginPlay(){Super::BeginPlay();if(HasAuthority()){Health=MaxHealth;if(!Controller)SpawnDefaultController();}}

float ABanditCharacter::TakeDamage(float Amount,const FDamageEvent& Event,AController* InstigatorController,AActor* Causer)
{
	if(!HasAuthority()||bDead||Amount<=0.f)return 0.f;const float Applied=Super::TakeDamage(Amount,Event,InstigatorController,Causer);if(Applied<=0.f)return 0.f;
	Health=FMath::Max(0.f,Health-Applied);if(Health<=0.f){bDead=true;MulticastDie();SetLifeSpan(18.f);}return Applied;
}

bool ABanditCharacter::FireAt(AActor* Target)
{
	if(!HasAuthority()||bDead||!Target||!ProjectileClass)return false;
	const FName MuzzleSocket(TEXT("b_gun_muzzleflash"));const FVector Muzzle=WeaponMesh&&WeaponMesh->DoesSocketExist(MuzzleSocket)?WeaponMesh->GetSocketLocation(MuzzleSocket):GetActorLocation()+GetActorForwardVector()*75.f+FVector(0,0,55.f);
	const FVector TargetPoint=Target->GetActorLocation()+FVector(0,0,45.f);const FVector Direction=FMath::VRandCone((TargetPoint-Muzzle).GetSafeNormal(),FMath::DegreesToRadians(SpreadDegrees));
	FActorSpawnParameters Params;Params.Owner=this;Params.Instigator=this;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if(ABallisticProjectile* Bullet=GetWorld()->SpawnActor<ABallisticProjectile>(ProjectileClass,Muzzle,Direction.Rotation(),Params))
	{
		Bullet->InitializeProjectile(BulletDamage,2.f,.7f,.2f,.7f,1.f,8.f,GetController());Bullet->Movement->Velocity=Direction*BulletSpeed;
		UAISense_Hearing::ReportNoiseEvent(GetWorld(),GetActorLocation(),1.f,this,20000.f,TEXT("Gunshot"));
		for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It)
		{
			AZombieCharacter* Zombie=*It;
			if(!Zombie||Zombie->IsDead()||FVector::DistSquared2D(GetActorLocation(),Zombie->GetActorLocation())>FMath::Square(20000.f))continue;
			if(AZombieAIController* ZombieController=Cast<AZombieAIController>(Zombie->GetController()))ZombieController->AlertToActor(this);
		}
		return true;
	}
	return false;
}

bool ABanditCharacter::TryMelee(AActor* Target)
{
	if(!HasAuthority()||bDead||!Target||GetWorld()->GetTimeSeconds()-LastMeleeTime<MeleeCooldown)return false;
	if(FVector::DistSquared(GetActorLocation(),Target->GetActorLocation())>FMath::Square(190.f))return false;LastMeleeTime=GetWorld()->GetTimeSeconds();
	if(UHealthArmorComponent* HealthComponent=Target->FindComponentByClass<UHealthArmorComponent>())return HealthComponent->ApplyDamage(MeleeDamage,GetController(),this)>0.f;
	return false;
}

void ABanditCharacter::MulticastDie_Implementation()
{
	GetCharacterMovement()->DisableMovement();GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if(GetMesh()){GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));GetMesh()->SetSimulatePhysics(true);}
}

void ABanditCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ABanditCharacter,Health);DOREPLIFETIME(ABanditCharacter,bDead);}
