#include "ShooterPlayerController.h"

#include "GameplayPanels.h"
#include "ShooterCharacter.h"
#include "ShooterGameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if(!InputComponent)return;
	FInputActionBinding& PauseBinding=InputComponent->BindAction(TEXT("Pause"),IE_Pressed,this,&AShooterPlayerController::TogglePauseMenu);
	PauseBinding.bExecuteWhenPaused=true;
	InputComponent->BindAction(TEXT("Inventory"),IE_Pressed,this,&AShooterPlayerController::ToggleInventory);
	InputComponent->BindAction(TEXT("SkillTree"),IE_Pressed,this,&AShooterPlayerController::ToggleSkillTree);
	InputComponent->BindAction(TEXT("BuildBed"),IE_Pressed,this,&AShooterPlayerController::ToggleBuildingMenu);
	InputComponent->BindAction(TEXT("Crafting"),IE_Pressed,this,&AShooterPlayerController::ToggleCrafting);
}

void AShooterPlayerController::TogglePauseMenu()
{
	if(IsGameplayPanelOpen())
	{
		CloseGameplayPanel();
		return;
	}
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->TogglePauseMenu();
}

bool AShooterPlayerController::IsGameplayPanelOpen()const
{
	return ActiveGameplayPanel&&ActiveGameplayPanel->IsInViewport();
}

void AShooterPlayerController::ToggleInventory()
{
	if(IsGameplayPanelOpen()){CloseGameplayPanel();return;}
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())if(GI->IsPauseMenuOpen())return;
	OpenGameplayPanel(UInventoryWidget::StaticClass());
}

void AShooterPlayerController::ToggleSkillTree()
{
	if(IsGameplayPanelOpen()){CloseGameplayPanel();return;}
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())if(GI->IsPauseMenuOpen())return;
	OpenGameplayPanel(USkillTreeWidget::StaticClass());
}
void AShooterPlayerController::ToggleBuildingMenu()
{
	if(IsGameplayPanelOpen()){CloseGameplayPanel();return;}
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())if(GI->IsPauseMenuOpen())return;
	OpenGameplayPanel(UBuildingMenuWidget::StaticClass());
}
void AShooterPlayerController::ToggleCrafting()
{
	if(IsGameplayPanelOpen()){CloseGameplayPanel();return;}
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())if(GI->IsPauseMenuOpen())return;
	OpenGameplayPanel(UInventoryWidget::StaticClass());
}

void AShooterPlayerController::OpenGameplayPanel(TSubclassOf<UUserWidget> WidgetClass)
{
	if(!IsLocalController()||!WidgetClass)return;
	CloseGameplayPanel();
	if(AShooterCharacter* ShooterPawn=Cast<AShooterCharacter>(GetPawn()))ShooterPawn->StopGameplayActionsForMenu();
	ActiveGameplayPanel=CreateWidget<UUserWidget>(this,WidgetClass);
	if(!ActiveGameplayPanel)return;
	ActiveGameplayPanel->AddToViewport(450);
	bShowMouseCursor=true;bEnableClickEvents=true;bEnableMouseOverEvents=true;
	SetIgnoreMoveInput(true);SetIgnoreLookInput(true);
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ActiveGameplayPanel->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	ActiveGameplayPanel->SetKeyboardFocus();
}

void AShooterPlayerController::OpenStorageChest(AStorageChest* Chest)
{
	if(!Chest||!IsLocalController())return;
	// OpenGameplayPanel closes the previous panel first, and CloseGameplayPanel
	// clears OpenChest. Assign the chest after that cleanup so the storage widget
	// can populate both grids on its first tick.
	OpenGameplayPanel(UStorageChestWidget::StaticClass());OpenChest=Chest;
}

void AShooterPlayerController::CloseGameplayPanel()
{
	if(ActiveGameplayPanel){ActiveGameplayPanel->RemoveFromParent();ActiveGameplayPanel=nullptr;}
	OpenChest=nullptr;
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())if(GI->IsPauseMenuOpen())return;
	bShowMouseCursor=false;bEnableClickEvents=false;bEnableMouseOverEvents=false;
	ResetIgnoreMoveInput();ResetIgnoreLookInput();
	FInputModeGameOnly InputMode;SetInputMode(InputMode);
}
