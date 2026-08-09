#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS()
class MYPROJECT_API ULanSessionEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Setup(int32 InIndex,const FString& InLabel);
protected:
	virtual void NativeOnInitialized()override;
private:
	UFUNCTION()void JoinClicked();
	int32 ResultIndex=INDEX_NONE;
	FString Label;
	UPROPERTY()UTextBlock* LabelText=nullptr;
};

UCLASS()
class MYPROJECT_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized()override;
	virtual void NativeDestruct()override;
private:
	UButton* AddButton(UVerticalBox* Parent,const FString& Label);
	void RebuildSessionList();
	UFUNCTION()void NewGameClicked();
	UFUNCTION()void ContinueClicked();
	UFUNCTION()void HostLanClicked();
	UFUNCTION()void FindLanClicked();
	UFUNCTION()void RefreshMenuState();
	UPROPERTY()UButton* ContinueButton=nullptr;
	UPROPERTY()UVerticalBox* SessionList=nullptr;
	UPROPERTY()UTextBlock* StatusText=nullptr;
};
