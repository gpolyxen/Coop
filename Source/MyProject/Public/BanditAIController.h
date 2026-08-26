#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BanditAIController.generated.h"

UCLASS()
class MYPROJECT_API ABanditAIController : public AAIController
{
	GENERATED_BODY()
public:
	ABanditAIController();
	virtual void Tick(float DeltaSeconds)override;
protected:
	AActor* FindTarget()const;
	bool FindCover(AActor* Target,FVector& OutCover)const;
	TWeakObjectPtr<AActor> TargetActor;
	FVector CoverLocation=FVector::ZeroVector;
	double CoverExpires=0.;double NextShotTime=0.;double RecrouchTime=0.;double NextTargetScan=0.;
	UPROPERTY(EditDefaultsOnly,Category="Bandit AI")float EngagementRange=4500.f;
	UPROPERTY(EditDefaultsOnly,Category="Bandit AI")float CoverSearchRadius=1800.f;
	UPROPERTY(EditDefaultsOnly,Category="Bandit AI")float MeleeRange=185.f;
};
