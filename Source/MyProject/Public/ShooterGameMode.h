#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

UCLASS()
class MYPROJECT_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AShooterGameMode();
protected:
	virtual void BeginPlay()override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player)override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)override;
private:
	AActor* EnsurePlayerStart();
	void FixStartupPlacements();
	bool FindProceduralGround(const FVector& Location,FVector& OutGround)const;
	FTimerHandle StartupGroundingTimer;
	int32 StartupGroundingPasses=0;
};
