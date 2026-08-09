#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS()
class MYPROJECT_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetSinglePlayerPaused(bool bInSinglePlayerPaused);
protected:
	virtual void NativeOnInitialized()override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& InKeyEvent)override;
private:
	UButton* AddButton(UVerticalBox* Parent,const FText& Label);
	UFUNCTION()void ContinueClicked();
	UFUNCTION()void MainMenuClicked();
	UFUNCTION()void ExitWindowsClicked();
	UPROPERTY()UTextBlock* ModeText=nullptr;
	bool bSinglePlayerPaused=false;
};
