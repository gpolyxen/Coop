#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

UCLASS()
class MYPROJECT_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()
protected:
	virtual void SetupInputComponent()override;
private:
	void TogglePauseMenu();
};
