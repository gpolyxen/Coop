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
public:AZombieAIController();virtual void Tick(float DeltaSeconds)override;
protected:virtual void OnPossess(APawn* InPawn)override;
	UPROPERTY(VisibleAnywhere)UAIPerceptionComponent* Senses;UPROPERTY()UAISenseConfig_Sight* Sight;UPROPERTY()UAISenseConfig_Hearing* Hearing;
	UFUNCTION()void OnTargetPerception(AActor* Actor,FAIStimulus Stimulus);TWeakObjectPtr<AActor> Target;FVector LastKnownLocation;float ForgetAfter=8.f;double LastStimulus=-1000.;
};
