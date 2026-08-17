#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

class UUserWidget;
class AStorageChest;

UCLASS()
class MYPROJECT_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void CloseGameplayPanel();
	bool IsGameplayPanelOpen()const;
	void OpenStorageChest(AStorageChest* Chest);
	AStorageChest* GetOpenStorageChest()const{return OpenChest;}
protected:
	virtual void SetupInputComponent()override;
private:
	void TogglePauseMenu();
	void ToggleInventory();
	void ToggleSkillTree();
	void ToggleBuildingMenu();
	void ToggleCrafting();
	void OpenGameplayPanel(TSubclassOf<UUserWidget> WidgetClass);
	UPROPERTY(Transient)UUserWidget* ActiveGameplayPanel=nullptr;
	UPROPERTY(Transient)AStorageChest* OpenChest=nullptr;
};
