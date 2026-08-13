#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterTypes.h"
#include "BuildTypes.h"
#include "GameplayPanels.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS()
class MYPROJECT_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized()override;
	virtual void NativeTick(const FGeometry& MyGeometry,float InDeltaTime)override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& InKeyEvent)override;
private:
	void Refresh();
	UFUNCTION()void CloseClicked();
	UPROPERTY()UTextBlock* CapacityText=nullptr;
	UPROPERTY()UVerticalBox* ItemList=nullptr;
	float NextRefreshTime=0.f;
};

UCLASS()
class MYPROJECT_API USkillTreeEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Setup(EShooterSkill InSkill);
	void Refresh();
protected:
	virtual void NativeOnInitialized()override;
private:
	UFUNCTION()void PurchaseClicked();
	UPROPERTY()UButton* PurchaseButton=nullptr;
	UPROPERTY()UTextBlock* Label=nullptr;
	EShooterSkill Skill=EShooterSkill::QuickReload;
};

UCLASS()
class MYPROJECT_API USkillTreeWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized()override;
	virtual void NativeTick(const FGeometry& MyGeometry,float InDeltaTime)override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& InKeyEvent)override;
private:
	void Refresh();
	UFUNCTION()void CloseClicked();
	UPROPERTY()UTextBlock* PointsText=nullptr;
	UPROPERTY()TArray<USkillTreeEntryWidget*> Entries;
	float NextRefreshTime=0.f;
};

UCLASS()
class MYPROJECT_API UBuildingMenuWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized()override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& InKeyEvent)override;
private:
	void ShowCategories();void ShowFurniture();void ShowDefense();void SelectBuildPiece(EBuildPieceType Type);
	UFUNCTION()void FurnitureClicked();UFUNCTION()void DefenseClicked();UFUNCTION()void BackClicked();
	UFUNCTION()void BedClicked();UFUNCTION()void WallClicked();UFUNCTION()void GateClicked();UFUNCTION()void FloorClicked();UFUNCTION()void StairsClicked();UFUNCTION()void CloseClicked();
	UPROPERTY()UVerticalBox* Content=nullptr;
};
