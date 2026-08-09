#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuGameMode.generated.h"

UCLASS()
class MYPROJECT_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay()override;
};

UCLASS()
class MYPROJECT_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AMainMenuGameMode();
};
