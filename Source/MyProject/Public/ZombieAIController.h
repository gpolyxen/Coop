#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "ZombieAIController.generated.h"
class UAIPerceptionComponent;class UAISenseConfig_Sight;class UAISenseConfig_Hearing;
UCLASS()
class MYPROJECT_API AZombieAIController:public AAIController
{
	GENERATED_BODY()
public:
	AZombieAIController();
	virtual void Tick(float DeltaSeconds)override;
	void AlertToActor(AActor* Actor);
protected:virtual void OnPossess(APawn* InPawn)override;
	UPROPERTY(VisibleAnywhere)UAIPerceptionComponent* Senses;UPROPERTY()UAISenseConfig_Sight* Sight;UPROPERTY()UAISenseConfig_Hearing* Hearing;
	UFUNCTION()void OnTargetPerception(AActor* Actor,FAIStimulus Stimulus);
	void TryStartPatrol();
	void TryAcquireVisibleTarget();
	void SteerToward(const FVector& Destination);
	bool TryJumpObstacle(const FVector& Destination);
	bool TryStartDetour(const FVector& Destination);
	void ResetNavigationRecovery();
	TWeakObjectPtr<AActor> Target;
	FVector LastKnownLocation;
	UPROPERTY(EditDefaultsOnly,Category="AI")float ForgetAfter=8.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolRadius=1500.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolAcceptanceRadius=80.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolWaitMin=1.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolWaitMax=4.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Navigation",meta=(ClampMin="0.1"))float PathRefreshInterval=.65f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Navigation",meta=(ClampMin="0.5"))float StuckRecoveryDelay=1.25f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Navigation",meta=(ClampMin="100"))float DetourSearchRadius=450.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Navigation",meta=(ClampMin="1.0"))float DetourTimeout=3.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="60"))float JumpProbeDistance=145.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="1"))float MinimumJumpObstacleHeight=35.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="20"))float MaximumJumpObstacleHeight=105.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="100"))float JumpForwardVelocity=430.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="0.1"))float JumpCooldown=1.2f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="0"))float JumpClearance=12.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="0"))float MaximumLandingDrop=90.f;
	double LastStimulus=-1000.;
	double NextPatrolTime=0.;
	bool bPatrolMoveActive=false;
	bool bSteeringPatrol=false;
	FVector SteeringPatrolLocation=FVector::ZeroVector;
	FVector LastChaseProgressLocation=FVector::ZeroVector;
	double LastChaseProgressTime=0.;
	double NextPathRefreshTime=0.;
	double LastJumpTime=-1000.;
	double DetourExpireTime=0.;
	bool bUsingDetour=false;
	FVector DetourLocation=FVector::ZeroVector;
	FVector JumpTravelDirection=FVector::ZeroVector;
	bool bLoggedInitialSightScan=false;
};
