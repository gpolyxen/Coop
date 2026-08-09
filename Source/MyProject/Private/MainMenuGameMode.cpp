#include "MainMenuGameMode.h"

#include "MainMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SpectatorPawn.h"

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if(!IsLocalController())return;
	UMainMenuWidget* Menu=CreateWidget<UMainMenuWidget>(this,UMainMenuWidget::StaticClass());
	if(Menu)Menu->AddToViewport(100);
	bShowMouseCursor=true;
	bEnableClickEvents=true;
	bEnableMouseOverEvents=true;
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
}

AMainMenuGameMode::AMainMenuGameMode()
{
	PlayerControllerClass=AMainMenuPlayerController::StaticClass();
	DefaultPawnClass=ASpectatorPawn::StaticClass();
	SpectatorClass=ASpectatorPawn::StaticClass();
	HUDClass=AHUD::StaticClass();
	bStartPlayersAsSpectators=true;
}
