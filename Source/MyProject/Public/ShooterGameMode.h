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
protected:virtual void BeginPlay()override;
};
