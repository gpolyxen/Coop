#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "ShooterTypes.h"
#include "BuildTypes.h"
#include "GameplayPanels.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UUniformGridPanel;
class UInventoryWidget;
class AStorageChest;

UCLASS()
class MYPROJECT_API UItemDragOperation : public UDragDropOperation
{
	GENERATED_BODY()
public:UPROPERTY()FName ItemId=NAME_None;
	UPROPERTY()int32 Quantity=0;
	UPROPERTY()bool bFromChest=false;
};

UCLASS()
class MYPROJECT_API UInventoryItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:void Setup(FName InItemId,int32 InQuantity,int32 InSlotIndex);
protected:virtual void NativeOnInitialized()override;virtual FReply NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)override;virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& G,const FPointerEvent& E)override;virtual void NativeOnDragDetected(const FGeometry& G,const FPointerEvent& E,UDragDropOperation*& Operation)override;
private:void RefreshLabel();UPROPERTY()UTextBlock* Label=nullptr;FName ItemId=NAME_None;int32 Quantity=0;int32 SlotIndex=INDEX_NONE;float LastLeftClickTime=-100.f;
};

UCLASS()
class MYPROJECT_API UCraftingIngredientSlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:void Setup(UInventoryWidget* InOwner,int32 InIndex);void SetIngredient(FName InItemId);
protected:virtual void NativeOnInitialized()override;virtual bool NativeOnDrop(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation)override;virtual FReply NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)override;
private:void RefreshLabel();UPROPERTY()UTextBlock* Label=nullptr;UPROPERTY()UInventoryWidget* OwnerWidget=nullptr;FName ItemId=NAME_None;int32 Index=INDEX_NONE;
};

UCLASS()
class MYPROJECT_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
public:void SetCraftIngredient(int32 Index,FName ItemId);
protected:
	virtual void NativeOnInitialized()override;
	virtual void NativeTick(const FGeometry& MyGeometry,float InDeltaTime)override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& InKeyEvent)override;
private:
	void Refresh();
	UFUNCTION()void CloseClicked();
	UFUNCTION()void CraftSelectedClicked();
	UPROPERTY()UTextBlock* CapacityText=nullptr;
	UPROPERTY()UUniformGridPanel* InventoryGrid=nullptr;
	UPROPERTY()UUniformGridPanel* CraftGrid=nullptr;
	UPROPERTY()UVerticalBox* RecipeList=nullptr;
	UPROPERTY()UButton* CraftSelectedButton=nullptr;
	UPROPERTY()TArray<UCraftingIngredientSlotWidget*> CraftSlots;
	TArray<FName> CraftIngredients;
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
	void ShowCategories();void ShowFurniture();void ShowDefense();void ShowLighting();void SelectBuildPiece(EBuildPieceType Type);
	UFUNCTION()void FurnitureClicked();UFUNCTION()void DefenseClicked();UFUNCTION()void LightingClicked();UFUNCTION()void BackClicked();
	UFUNCTION()void BedClicked();UFUNCTION()void ChestClicked();UFUNCTION()void WallClicked();UFUNCTION()void GateClicked();UFUNCTION()void FloorClicked();UFUNCTION()void StairsClicked();UFUNCTION()void PillarClicked();UFUNCTION()void TorchClicked();UFUNCTION()void CloseClicked();
	UPROPERTY()UVerticalBox* Content=nullptr;
};

UCLASS()
class MYPROJECT_API UCraftingWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized()override;
	virtual void NativeTick(const FGeometry& MyGeometry,float InDeltaTime)override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& InKeyEvent)override;
private:
	void Refresh();
	UFUNCTION()void CraftMedkitClicked();
	UFUNCTION()void UseMedkitClicked();
	UFUNCTION()void CloseClicked();
	UPROPERTY()UTextBlock* MaterialsText=nullptr;
	UPROPERTY()UButton* CraftButton=nullptr;
	UPROPERTY()UButton* UseButton=nullptr;
	float NextRefreshTime=0.f;
};

UCLASS()
class MYPROJECT_API UStorageTransferSlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:void Setup(FName InItemId,int32 InQuantity,bool bInFromChest);
protected:virtual void NativeOnInitialized()override;virtual FReply NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)override;virtual void NativeOnDragDetected(const FGeometry& G,const FPointerEvent& E,UDragDropOperation*& Operation)override;virtual bool NativeOnDrop(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation)override;
private:UPROPERTY()UTextBlock* Label=nullptr;FName ItemId=NAME_None;int32 Quantity=0;bool bFromChest=false;
};

UCLASS()
class MYPROJECT_API UStorageChestWidget : public UUserWidget
{
	GENERATED_BODY()
protected:virtual void NativeOnInitialized()override;virtual void NativeTick(const FGeometry& G,float Dt)override;virtual FReply NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)override;
private:void Refresh();UFUNCTION()void StoreWeaponClicked();UFUNCTION()void CloseClicked();UPROPERTY()UUniformGridPanel* PlayerGrid=nullptr;UPROPERTY()UUniformGridPanel* ChestGrid=nullptr;UPROPERTY()UButton* StoreWeaponButton=nullptr;float NextRefreshTime=0.f;
};
