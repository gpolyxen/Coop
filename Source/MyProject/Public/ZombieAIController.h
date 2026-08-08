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
	TWeakObjectPtr<AActor> Target;
	FVector LastKnownLocation;
	UPROPERTY(EditDefaultsOnly,Category="AI")float ForgetAfter=8.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolRadius=1500.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolAcceptanceRadius=80.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolWaitMin=1.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolWaitMax=4.f;
	double LastStimulus=-1000.;
	double NextPatrolTime=0.;
	bool bPatrolMoveActive=false;
	bool bSteeringPatrol=false;
	FVector SteeringPatrolLocation=FVector::ZeroVector;
	FVector LastChaseProgressLocation=FVector::ZeroVector;
	double LastChaseProgressTime=0.;
	bool bDirectChase=false;
	bool bLoggedInitialSightScan=false;
};
