#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

class UUserWidget;

UCLASS()
class MYPROJECT_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void CloseGameplayPanel();
	bool IsGameplayPanelOpen()const;
protected:
	virtual void SetupInputComponent()override;
private:
	void TogglePauseMenu();
	void ToggleInventory();
	void ToggleSkillTree();
	void OpenGameplayPanel(TSubclassOf<UUserWidget> WidgetClass);
	UPROPERTY(Transient)UUserWidget* ActiveGameplayPanel=nullptr;
};
