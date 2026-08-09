#include "ShooterPlayerController.h"

#include "ShooterGameInstance.h"
#include "Components/InputComponent.h"

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if(!InputComponent)return;
	FInputActionBinding& PauseBinding=InputComponent->BindAction(TEXT("Pause"),IE_Pressed,this,&AShooterPlayerController::TogglePauseMenu);
	PauseBinding.bExecuteWhenPaused=true;
}

void AShooterPlayerController::TogglePauseMenu()
{
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->TogglePauseMenu();
}
