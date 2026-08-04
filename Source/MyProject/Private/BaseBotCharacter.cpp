#include "BaseBotCharacter.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h" 

ABaseBotCharacter::ABaseBotCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// AI должен автоматически получать контроллер.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Бот поворачивается по направлению движения.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (Movement)
	{
		Movement->bOrientRotationToMovement = true;

		// Скорость поворота. Позже можно настроить в Blueprint.
		Movement->RotationRate = FRotator(0.0f, 360.0f, 0.0f);

		Movement->MaxWalkSpeed = 400.0f;
		Movement->MaxAcceleration = 1000.0f;
		Movement->BrakingDecelerationWalking = 800.0f;
		Movement->GroundFriction = 6.0f;

		// Позволяет AI учитывать движение по краям NavMesh.
		Movement->bUseControllerDesiredRotation = false;

		// Параметры прыжка.
		Movement->JumpZVelocity = 500.0f;
		Movement->AirControl = 0.25f;
	}
}

void ABaseBotCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsDead = false;
}

void ABaseBotCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

float ABaseBotCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser
	);

	if (AppliedDamage <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Clamp(
		CurrentHealth - AppliedDamage,
		0.0f,
		MaxHealth
	);

	BP_OnDamageReceived(AppliedDamage, DamageCauser);

	// Проверяем, пришёл ли урон именно от Apply Point Damage.
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamageEvent =
			static_cast<const FPointDamageEvent*>(&DamageEvent);

		BP_OnPointDamageReceived(
			AppliedDamage,
			PointDamageEvent->HitInfo.ImpactPoint,
			PointDamageEvent->ShotDirection,
			PointDamageEvent->HitInfo.BoneName,
			DamageCauser
		);
	}

	if (CurrentHealth <= 0.0f)
	{
		Die(EventInstigator, DamageCauser);
	}

	return AppliedDamage;
}

bool ABaseBotCharacter::IsDead() const
{
	return bIsDead;
}

float ABaseBotCharacter::GetHealthPercent() const
{
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}

	return CurrentHealth / MaxHealth;
}

void ABaseBotCharacter::Die(
	AController* KillerController,
	AActor* DamageCauser)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	CurrentHealth = 0.0f;

	// Больше не даём AI перемещать персонажа.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	// Отключаем столкновение капсулы.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	DetachFromControllerPendingDestroy();

	BP_OnDeath(KillerController, DamageCauser);
}