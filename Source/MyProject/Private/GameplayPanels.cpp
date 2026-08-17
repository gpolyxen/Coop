#include "GameplayPanels.h"

#include "InventoryComponent.h"
#include "HealthArmorComponent.h"
#include "ShooterCharacter.h"
#include "ShooterPlayerController.h"
#include "WeaponBase.h"
#include "StorageChest.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree,const FText& Text,int32 Size,const FLinearColor& Color,ETextJustify::Type Justification=ETextJustify::Left)
	{
		UTextBlock* Result=Tree->ConstructWidget<UTextBlock>();
		Result->SetText(Text);Result->SetColorAndOpacity(FSlateColor(Color));Result->SetJustification(Justification);
		FSlateFontInfo Font=Result->Font;Font.Size=Size;Result->SetFont(Font);Result->SetAutoWrapText(true);
		return Result;
	}

	UButton* MakeButton(UWidgetTree* Tree,const FText& Text)
	{
		UButton* Button=Tree->ConstructWidget<UButton>();Button->SetBackgroundColor(FLinearColor(.08f,.3f,.42f,.96f));
		UTextBlock* Label=MakeText(Tree,Text,18,FLinearColor::White,ETextJustify::Center);Button->AddChild(Label);
		if(UButtonSlot* Slot=Cast<UButtonSlot>(Label->Slot))Slot->SetPadding(FMargin(14.f,8.f));
		return Button;
	}

	UVerticalBox* CreatePanel(UWidgetTree* Tree,const FText& Title,float Width)
	{
		UOverlay* Root=Tree->ConstructWidget<UOverlay>();Tree->RootWidget=Root;
		UBorder* Dim=Tree->ConstructWidget<UBorder>();Dim->SetBrushColor(FLinearColor(0.f,0.f,0.f,.58f));
		if(UOverlaySlot* Slot=Root->AddChildToOverlay(Dim)){Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Fill);}
		USizeBox* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(Width);Size->SetMaxDesiredHeight(720.f);
		if(UOverlaySlot* Slot=Root->AddChildToOverlay(Size)){Slot->SetHorizontalAlignment(HAlign_Center);Slot->SetVerticalAlignment(VAlign_Center);}
		UBorder* Border=Tree->ConstructWidget<UBorder>();Border->SetBrushColor(FLinearColor(.018f,.055f,.075f,.98f));Border->SetPadding(FMargin(25.f));Size->AddChild(Border);
		UVerticalBox* Panel=Tree->ConstructWidget<UVerticalBox>();Border->AddChild(Panel);
		UTextBlock* Header=MakeText(Tree,Title,34,FLinearColor(.65f,.9f,1.f),ETextJustify::Center);
		if(UVerticalBoxSlot* Slot=Panel->AddChildToVerticalBox(Header))Slot->SetPadding(FMargin(5.f,3.f,5.f,15.f));
		return Panel;
	}

	AShooterCharacter* GetCharacter(const UUserWidget* Widget){return Widget?Cast<AShooterCharacter>(Widget->GetOwningPlayerPawn()):nullptr;}
}

void UInventoryItemSlotWidget::Setup(FName InItemId,int32 InQuantity,int32 InSlotIndex){ItemId=InItemId;Quantity=InQuantity;SlotIndex=InSlotIndex;RefreshLabel();}
void UInventoryItemSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	UBorder* Border=WidgetTree->ConstructWidget<UBorder>();WidgetTree->RootWidget=Border;Border->SetBrushColor(ItemId.IsNone()?FLinearColor(.03f,.07f,.09f,.9f):FLinearColor(.08f,.22f,.28f,.98f));Border->SetPadding(FMargin(5.f));
	USizeBox* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(92.f);Size->SetHeightOverride(92.f);Border->AddChild(Size);
	Label=MakeText(WidgetTree,FText::GetEmpty(),14,FLinearColor::White,ETextJustify::Center);Size->AddChild(Label);RefreshLabel();
}
void UInventoryItemSlotWidget::RefreshLabel(){if(!Label)return;if(ItemId.IsNone())Label->SetText(FText::FromString(TEXT("ПУСТО")));else if(ItemId==TEXT("Medkit"))Label->SetText(FText::FromString(FString::Printf(TEXT("%s\nx%d\nДВОЙНОЙ КЛИК"),*ItemId.ToString(),Quantity)));else Label->SetText(FText::FromString(FString::Printf(TEXT("%s\nx%d"),*ItemId.ToString(),Quantity)));}
FReply UInventoryItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E){if(ItemId==TEXT("Medkit")&&E.GetEffectingButton()==EKeys::LeftMouseButton&&GetWorld()){const float Now=GetWorld()->GetRealTimeSeconds();if(Now-LastLeftClickTime<=.4f){if(AShooterCharacter* Character=GetCharacter(this))Character->UseMedkit();LastLeftClickTime=-100.f;return FReply::Handled();}LastLeftClickTime=Now;}if(!ItemId.IsNone()&&E.GetEffectingButton()==EKeys::LeftMouseButton)return UWidgetBlueprintLibrary::DetectDragIfPressed(E,this,EKeys::LeftMouseButton).NativeReply;return Super::NativeOnMouseButtonDown(G,E);}
FReply UInventoryItemSlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& G,const FPointerEvent& E){if(ItemId==TEXT("Medkit")&&E.GetEffectingButton()==EKeys::LeftMouseButton){if(AShooterCharacter* Character=GetCharacter(this))Character->UseMedkit();return FReply::Handled();}return Super::NativeOnMouseButtonDoubleClick(G,E);}
void UInventoryItemSlotWidget::NativeOnDragDetected(const FGeometry&,const FPointerEvent&,UDragDropOperation*& Operation){if(ItemId.IsNone())return;UItemDragOperation* Drag=NewObject<UItemDragOperation>(this);Drag->ItemId=ItemId;Drag->Quantity=Quantity;Drag->bFromChest=false;Operation=Drag;}

void UCraftingIngredientSlotWidget::Setup(UInventoryWidget* InOwner,int32 InIndex){OwnerWidget=InOwner;Index=InIndex;RefreshLabel();}
void UCraftingIngredientSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	UBorder* Border=WidgetTree->ConstructWidget<UBorder>();WidgetTree->RootWidget=Border;Border->SetBrushColor(FLinearColor(.16f,.12f,.035f,.98f));Border->SetPadding(FMargin(5.f));
	USizeBox* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(78.f);Size->SetHeightOverride(78.f);Border->AddChild(Size);
	Label=MakeText(WidgetTree,FText::GetEmpty(),13,FLinearColor(1.f,.85f,.35f),ETextJustify::Center);Size->AddChild(Label);RefreshLabel();
}
void UCraftingIngredientSlotWidget::SetIngredient(FName InItemId){ItemId=InItemId;RefreshLabel();}
void UCraftingIngredientSlotWidget::RefreshLabel(){if(Label)Label->SetText(FText::FromString(ItemId.IsNone()?FString::Printf(TEXT("СЛОТ %d"),Index+1):ItemId.ToString()));}
bool UCraftingIngredientSlotWidget::NativeOnDrop(const FGeometry&,const FDragDropEvent&,UDragDropOperation* Operation){if(UItemDragOperation* Drag=Cast<UItemDragOperation>(Operation)){if(OwnerWidget)OwnerWidget->SetCraftIngredient(Index,Drag->ItemId);return true;}return false;}
FReply UCraftingIngredientSlotWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E){if(E.GetEffectingButton()==EKeys::RightMouseButton){if(OwnerWidget)OwnerWidget->SetCraftIngredient(Index,NAME_None);return FReply::Handled();}return Super::NativeOnMouseButtonDown(G,E);}

void UInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	UVerticalBox* Panel=CreatePanel(WidgetTree,FText::FromString(TEXT("ИНВЕНТАРЬ И КРАФТ  [I]")),1050.f);
	CapacityText=MakeText(WidgetTree,FText::GetEmpty(),20,FLinearColor(.55f,.85f,1.f),ETextJustify::Center);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(CapacityText))PanelSlot->SetPadding(FMargin(4.f,2.f,4.f,10.f));
	UHorizontalBox* Columns=WidgetTree->ConstructWidget<UHorizontalBox>();if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(Columns))PanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UVerticalBox* Left=WidgetTree->ConstructWidget<UVerticalBox>();if(UHorizontalBoxSlot* ColumnSlot=Columns->AddChildToHorizontalBox(Left)){ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ColumnSlot->SetPadding(FMargin(5.f,5.f,18.f,5.f));}
	Left->AddChildToVerticalBox(MakeText(WidgetTree,FText::FromString(TEXT("ИНВЕНТАРЬ — ПЕРЕТАЩИТЕ ПРЕДМЕТЫ В КРАФТ")),17,FLinearColor(.65f,.9f,1.f),ETextJustify::Center));
	InventoryGrid=WidgetTree->ConstructWidget<UUniformGridPanel>();if(UVerticalBoxSlot* GridSlot=Left->AddChildToVerticalBox(InventoryGrid))GridSlot->SetPadding(FMargin(4.f,12.f));
	UVerticalBox* Right=WidgetTree->ConstructWidget<UVerticalBox>();if(UHorizontalBoxSlot* ColumnSlot=Columns->AddChildToHorizontalBox(Right)){ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ColumnSlot->SetPadding(FMargin(18.f,5.f,5.f,5.f));}
	Right->AddChildToVerticalBox(MakeText(WidgetTree,FText::FromString(TEXT("КРАФТ — 8 ЯЧЕЕК")),19,FLinearColor(1.f,.78f,.25f),ETextJustify::Center));
	CraftGrid=WidgetTree->ConstructWidget<UUniformGridPanel>();if(UVerticalBoxSlot* GridSlot=Right->AddChildToVerticalBox(CraftGrid))GridSlot->SetPadding(FMargin(4.f,12.f));
	CraftIngredients.SetNum(8);for(int32 Index=0;Index<8;++Index){UCraftingIngredientSlotWidget* SlotWidget=CreateWidget<UCraftingIngredientSlotWidget>(GetOwningPlayer(),UCraftingIngredientSlotWidget::StaticClass());SlotWidget->Setup(this,Index);CraftSlots.Add(SlotWidget);CraftGrid->AddChildToUniformGrid(SlotWidget,Index/4,Index%4);}
	RecipeList=WidgetTree->ConstructWidget<UVerticalBox>();if(UVerticalBoxSlot* RecipeSlot=Right->AddChildToVerticalBox(RecipeList))RecipeSlot->SetPadding(FMargin(4.f,12.f));
	CraftSelectedButton=MakeButton(WidgetTree,FText::FromString(TEXT("СКРАФТИТЬ ВЫБРАННОЕ")));CraftSelectedButton->OnClicked.AddDynamic(this,&UInventoryWidget::CraftSelectedClicked);Right->AddChildToVerticalBox(CraftSelectedButton);
	UButton* Close=MakeButton(WidgetTree,FText::FromString(TEXT("ЗАКРЫТЬ")));Close->OnClicked.AddDynamic(this,&UInventoryWidget::CloseClicked);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(Close))PanelSlot->SetPadding(FMargin(4.f,14.f,4.f,2.f));
	Refresh();
}

void UInventoryWidget::NativeTick(const FGeometry& MyGeometry,float InDeltaTime)
{
	Super::NativeTick(MyGeometry,InDeltaTime);
	if(GetWorld()&&GetWorld()->GetTimeSeconds()>=NextRefreshTime){NextRefreshTime=GetWorld()->GetTimeSeconds()+.5f;Refresh();}
}

void UInventoryWidget::Refresh()
{
	AShooterCharacter* Character=GetCharacter(this);if(!Character||!Character->Inventory||!CapacityText||!InventoryGrid)return;
	UInventoryComponent* Inventory=Character->Inventory;
	CapacityText->SetText(FText::FromString(FString::Printf(TEXT("СЛОТЫ: %d / %d     МАКС. ВЕС: %.0f КГ"),Inventory->Items.Num(),Inventory->MaxSlots,Inventory->MaxWeight)));
	InventoryGrid->ClearChildren();
	for(int32 SlotIndex=0;SlotIndex<Inventory->MaxSlots;++SlotIndex)
	{
		const FName Item=Inventory->Items.IsValidIndex(SlotIndex)?Inventory->Items[SlotIndex].ItemId:NAME_None;const int32 Quantity=Inventory->Items.IsValidIndex(SlotIndex)?Inventory->Items[SlotIndex].Quantity:0;
		UInventoryItemSlotWidget* Cell=CreateWidget<UInventoryItemSlotWidget>(GetOwningPlayer(),UInventoryItemSlotWidget::StaticClass());Cell->Setup(Item,Quantity,SlotIndex);InventoryGrid->AddChildToUniformGrid(Cell,SlotIndex/5,SlotIndex%5);
	}
	int32 Medicine=0,Bandage=0;for(FName Ingredient:CraftIngredients){if(Ingredient==TEXT("Medicine"))++Medicine;else if(Ingredient==TEXT("Bandage"))++Bandage;}
	const bool bMedkitRecipe=Medicine>=1&&Bandage>=1;if(RecipeList){RecipeList->ClearChildren();RecipeList->AddChildToVerticalBox(MakeText(WidgetTree,FText::FromString(bMedkitRecipe?TEXT("ДОСТУПНО:\nАПТЕЧКА — 1 медикамент + 2 бинта"):TEXT("ДОСТУПНЫХ РЕЦЕПТОВ НЕТ")),17,bMedkitRecipe?FLinearColor(.35f,1.f,.45f):FLinearColor(.55f,.55f,.55f)));}
	if(CraftSelectedButton)CraftSelectedButton->SetIsEnabled(bMedkitRecipe&&Character->CanCraftMedkit());
}

void UInventoryWidget::SetCraftIngredient(int32 Index,FName ItemId){if(!CraftIngredients.IsValidIndex(Index))return;CraftIngredients[Index]=ItemId;if(CraftSlots.IsValidIndex(Index)&&CraftSlots[Index])CraftSlots[Index]->SetIngredient(ItemId);Refresh();}
void UInventoryWidget::CraftSelectedClicked(){AShooterCharacter* Character=GetCharacter(this);if(!Character)return;int32 Medicine=0,Bandage=0;for(FName Ingredient:CraftIngredients){if(Ingredient==TEXT("Medicine"))++Medicine;else if(Ingredient==TEXT("Bandage"))++Bandage;}if(Medicine>=1&&Bandage>=1)Character->CraftMedkit();Refresh();}

void UInventoryWidget::CloseClicked(){if(AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer()))PC->CloseGameplayPanel();}
FReply UInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& Event){if(Event.GetKey()==EKeys::I||Event.GetKey()==EKeys::Escape){CloseClicked();return FReply::Handled();}return Super::NativeOnKeyDown(InGeometry,Event);}

void USkillTreeEntryWidget::Setup(EShooterSkill InSkill){Skill=InSkill;Refresh();}
void USkillTreeEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	PurchaseButton=WidgetTree->ConstructWidget<UButton>();WidgetTree->RootWidget=PurchaseButton;PurchaseButton->SetBackgroundColor(FLinearColor(.07f,.22f,.3f,.98f));
	Label=MakeText(WidgetTree,FText::GetEmpty(),17,FLinearColor::White);PurchaseButton->AddChild(Label);
	if(UButtonSlot* PanelSlot=Cast<UButtonSlot>(Label->Slot))PanelSlot->SetPadding(FMargin(16.f,10.f));
	PurchaseButton->OnClicked.AddDynamic(this,&USkillTreeEntryWidget::PurchaseClicked);Refresh();
}
void USkillTreeEntryWidget::Refresh()
{
	AShooterCharacter* Character=GetCharacter(this);if(!Character||!Label||!PurchaseButton)return;
	const bool bOwned=Character->HasSkill(Skill);const int32 Cost=Character->GetSkillCost(Skill);
	const FString Requirement=Character->GetSkillRequirementText(Skill).ToString();
	const FString State=bOwned?TEXT("[ИЗУЧЕНО]"):FString::Printf(TEXT("[ЦЕНА: %d]"),Cost);
	Label->SetText(FText::FromString(FString::Printf(TEXT("%s  %s\n%s%s%s"),*State,*Character->GetSkillName(Skill).ToString(),*Character->GetSkillDescription(Skill).ToString(),Requirement.IsEmpty()?TEXT(""):TEXT("\n"),*Requirement)));
	PurchaseButton->SetIsEnabled(Character->CanPurchaseSkill(Skill));
	PurchaseButton->SetBackgroundColor(bOwned?FLinearColor(.08f,.38f,.18f,.98f):FLinearColor(.07f,.22f,.3f,.98f));
}
void USkillTreeEntryWidget::PurchaseClicked(){if(AShooterCharacter* Character=GetCharacter(this))Character->PurchaseSkill(Skill);Refresh();}

void USkillTreeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	UVerticalBox* Panel=CreatePanel(WidgetTree,FText::FromString(TEXT("ДЕРЕВО НАВЫКОВ  [K]")),780.f);
	PointsText=MakeText(WidgetTree,FText::GetEmpty(),22,FLinearColor(1.f,.78f,.2f),ETextJustify::Center);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(PointsText))PanelSlot->SetPadding(FMargin(4.f,2.f,4.f,10.f));
	UScrollBox* Scroll=WidgetTree->ConstructWidget<UScrollBox>();if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(Scroll))PanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UVerticalBox* List=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(List);
	for(uint8 Index=0;Index<=static_cast<uint8>(EShooterSkill::LastLife);++Index)
	{
		USkillTreeEntryWidget* Entry=CreateWidget<USkillTreeEntryWidget>(GetOwningPlayer(),USkillTreeEntryWidget::StaticClass());Entry->Setup(static_cast<EShooterSkill>(Index));Entries.Add(Entry);
		if(UVerticalBoxSlot* PanelSlot=List->AddChildToVerticalBox(Entry))PanelSlot->SetPadding(FMargin(Index>=5?32.f:5.f,4.f,5.f,4.f));
	}
	UButton* Close=MakeButton(WidgetTree,FText::FromString(TEXT("ЗАКРЫТЬ")));Close->OnClicked.AddDynamic(this,&USkillTreeWidget::CloseClicked);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(Close))PanelSlot->SetPadding(FMargin(4.f,14.f,4.f,2.f));Refresh();
}
void USkillTreeWidget::NativeTick(const FGeometry& MyGeometry,float InDeltaTime){Super::NativeTick(MyGeometry,InDeltaTime);if(GetWorld()&&GetWorld()->GetTimeSeconds()>=NextRefreshTime){NextRefreshTime=GetWorld()->GetTimeSeconds()+.2f;Refresh();}}
void USkillTreeWidget::Refresh(){if(AShooterCharacter* Character=GetCharacter(this)){if(PointsText)PointsText->SetText(FText::FromString(FString::Printf(TEXT("ДОСТУПНО ОЧКОВ: %d"),Character->SkillPoints)));for(USkillTreeEntryWidget* Entry:Entries)if(Entry)Entry->Refresh();}}
void USkillTreeWidget::CloseClicked(){if(AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer()))PC->CloseGameplayPanel();}
FReply USkillTreeWidget::NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& Event){if(Event.GetKey()==EKeys::K||Event.GetKey()==EKeys::Escape){CloseClicked();return FReply::Handled();}return Super::NativeOnKeyDown(InGeometry,Event);}

void UBuildingMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	UVerticalBox* Panel=CreatePanel(WidgetTree,FText::FromString(TEXT("СТРОИТЕЛЬСТВО  [B]")),700.f);
	Content=WidgetTree->ConstructWidget<UVerticalBox>();if(UVerticalBoxSlot* ContentSlot=Panel->AddChildToVerticalBox(Content))ContentSlot->SetPadding(FMargin(8.f));
	UButton* Close=MakeButton(WidgetTree,FText::FromString(TEXT("ЗАКРЫТЬ")));Close->OnClicked.AddDynamic(this,&UBuildingMenuWidget::CloseClicked);
	if(UVerticalBoxSlot* CloseSlot=Panel->AddChildToVerticalBox(Close))CloseSlot->SetPadding(FMargin(5.f,18.f,5.f,2.f));ShowCategories();
}
void UBuildingMenuWidget::ShowCategories()
{
	Content->ClearChildren();
	UTextBlock* Hint=MakeText(WidgetTree,FText::FromString(TEXT("ВЫБЕРИТЕ РАЗДЕЛ")),20,FLinearColor(.65f,.85f,1.f),ETextJustify::Center);Content->AddChildToVerticalBox(Hint);
	UButton* Furniture=MakeButton(WidgetTree,FText::FromString(TEXT("МЕБЕЛЬ\nКровати и предметы быта")));Furniture->OnClicked.AddDynamic(this,&UBuildingMenuWidget::FurnitureClicked);Content->AddChildToVerticalBox(Furniture);
	UButton* Defense=MakeButton(WidgetTree,FText::FromString(TEXT("ЗАЩИТА\nСтены и ворота")));Defense->OnClicked.AddDynamic(this,&UBuildingMenuWidget::DefenseClicked);Content->AddChildToVerticalBox(Defense);
	UButton* Lighting=MakeButton(WidgetTree,FText::FromString(TEXT("ОСВЕЩЕНИЕ\nНастенные источники света")));Lighting->OnClicked.AddDynamic(this,&UBuildingMenuWidget::LightingClicked);Content->AddChildToVerticalBox(Lighting);
}
void UBuildingMenuWidget::ShowFurniture(){Content->ClearChildren();UButton* Bed=MakeButton(WidgetTree,FText::FromString(TEXT("КРОВАТЬ\n10 палок + 10 верёвок\nСохранение игры")));Bed->OnClicked.AddDynamic(this,&UBuildingMenuWidget::BedClicked);Content->AddChildToVerticalBox(Bed);UButton* Chest=MakeButton(WidgetTree,FText::FromString(TEXT("СУНДУК\n20 ячеек | 20 дерева + 5 кожи")));Chest->OnClicked.AddDynamic(this,&UBuildingMenuWidget::ChestClicked);Content->AddChildToVerticalBox(Chest);UButton* Back=MakeButton(WidgetTree,FText::FromString(TEXT("НАЗАД К РАЗДЕЛАМ")));Back->OnClicked.AddDynamic(this,&UBuildingMenuWidget::BackClicked);Content->AddChildToVerticalBox(Back);}
void UBuildingMenuWidget::ShowDefense(){Content->ClearChildren();UButton* Wall=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННАЯ СТЕНА\n6 палок | 350 HP | модуль 3 м")));Wall->OnClicked.AddDynamic(this,&UBuildingMenuWidget::WallClicked);Content->AddChildToVerticalBox(Wall);UButton* Gate=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННЫЕ ВОРОТА\n12 палок + 4 верёвки | 500 HP | модуль 3 м")));Gate->OnClicked.AddDynamic(this,&UBuildingMenuWidget::GateClicked);Content->AddChildToVerticalBox(Gate);UButton* Floor=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННЫЙ ПОЛ\n5 палок | площадка 3 x 3 м\nДо 3 секций от несущей опоры")));Floor->OnClicked.AddDynamic(this,&UBuildingMenuWidget::FloorClicked);Content->AddChildToVerticalBox(Floor);UButton* Stairs=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННАЯ ЛЕСТНИЦА\n8 палок | подъём на один этаж")));Stairs->OnClicked.AddDynamic(this,&UBuildingMenuWidget::StairsClicked);Content->AddChildToVerticalBox(Stairs);UButton* Pillar=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННАЯ КОЛОННА\n4 дерева | опора для пола второго этажа")));Pillar->OnClicked.AddDynamic(this,&UBuildingMenuWidget::PillarClicked);Content->AddChildToVerticalBox(Pillar);UButton* Back=MakeButton(WidgetTree,FText::FromString(TEXT("НАЗАД К РАЗДЕЛАМ")));Back->OnClicked.AddDynamic(this,&UBuildingMenuWidget::BackClicked);Content->AddChildToVerticalBox(Back);}
void UBuildingMenuWidget::ShowLighting(){Content->ClearChildren();UButton* Torch=MakeButton(WidgetTree,FText::FromString(TEXT("НАСТЕННЫЙ ФАКЕЛ\n1 палка + 1 ткань + 1 бензин\nАвтоматически загорается ночью")));Torch->OnClicked.AddDynamic(this,&UBuildingMenuWidget::TorchClicked);Content->AddChildToVerticalBox(Torch);UButton* Back=MakeButton(WidgetTree,FText::FromString(TEXT("НАЗАД К РАЗДЕЛАМ")));Back->OnClicked.AddDynamic(this,&UBuildingMenuWidget::BackClicked);Content->AddChildToVerticalBox(Back);}
void UBuildingMenuWidget::SelectBuildPiece(EBuildPieceType Type){if(AShooterCharacter* Character=GetCharacter(this))Character->BeginBuildPlacement(Type);CloseClicked();}
void UBuildingMenuWidget::FurnitureClicked(){ShowFurniture();}void UBuildingMenuWidget::DefenseClicked(){ShowDefense();}void UBuildingMenuWidget::LightingClicked(){ShowLighting();}void UBuildingMenuWidget::BackClicked(){ShowCategories();}
void UBuildingMenuWidget::BedClicked(){SelectBuildPiece(EBuildPieceType::Bed);}void UBuildingMenuWidget::ChestClicked(){SelectBuildPiece(EBuildPieceType::WoodChest);}void UBuildingMenuWidget::WallClicked(){SelectBuildPiece(EBuildPieceType::WoodWall);}void UBuildingMenuWidget::GateClicked(){SelectBuildPiece(EBuildPieceType::WoodGate);}void UBuildingMenuWidget::FloorClicked(){SelectBuildPiece(EBuildPieceType::WoodFloor);}void UBuildingMenuWidget::StairsClicked(){SelectBuildPiece(EBuildPieceType::WoodStairs);}void UBuildingMenuWidget::PillarClicked(){SelectBuildPiece(EBuildPieceType::WoodPillar);}void UBuildingMenuWidget::TorchClicked(){SelectBuildPiece(EBuildPieceType::WallTorch);}
void UBuildingMenuWidget::CloseClicked(){if(AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer()))PC->CloseGameplayPanel();}
FReply UBuildingMenuWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::B||E.GetKey()==EKeys::Escape){CloseClicked();return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}

void UCraftingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	UVerticalBox* Panel=CreatePanel(WidgetTree,FText::FromString(TEXT("КРАФТ  [C]")),650.f);
	MaterialsText=MakeText(WidgetTree,FText::GetEmpty(),20,FLinearColor(.75f,.9f,1.f),ETextJustify::Center);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(MaterialsText))PanelSlot->SetPadding(FMargin(4.f,4.f,4.f,16.f));
	CraftButton=MakeButton(WidgetTree,FText::FromString(TEXT("АПТЕЧКА\n1 медикамент + 2 бинта\nВосстанавливает 50 HP")));CraftButton->OnClicked.AddDynamic(this,&UCraftingWidget::CraftMedkitClicked);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(CraftButton))PanelSlot->SetPadding(FMargin(4.f,5.f));
	UseButton=MakeButton(WidgetTree,FText::FromString(TEXT("ИСПОЛЬЗОВАТЬ АПТЕЧКУ")));UseButton->OnClicked.AddDynamic(this,&UCraftingWidget::UseMedkitClicked);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(UseButton))PanelSlot->SetPadding(FMargin(4.f,12.f,4.f,5.f));
	UButton* Close=MakeButton(WidgetTree,FText::FromString(TEXT("ЗАКРЫТЬ")));Close->OnClicked.AddDynamic(this,&UCraftingWidget::CloseClicked);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(Close))PanelSlot->SetPadding(FMargin(4.f,18.f,4.f,2.f));Refresh();
}
void UCraftingWidget::NativeTick(const FGeometry& G,float Dt){Super::NativeTick(G,Dt);if(GetWorld()&&GetWorld()->GetTimeSeconds()>=NextRefreshTime){NextRefreshTime=GetWorld()->GetTimeSeconds()+.2f;Refresh();}}
void UCraftingWidget::Refresh()
{
	AShooterCharacter* Character=GetCharacter(this);if(!Character||!Character->Inventory)return;
	const int32 Medicine=Character->Inventory->GetQuantity(TEXT("Medicine")),Bandages=Character->Inventory->GetQuantity(TEXT("Bandage")),Medkits=Character->Inventory->GetQuantity(TEXT("Medkit"));
	if(MaterialsText)MaterialsText->SetText(FText::FromString(FString::Printf(TEXT("МЕДИКАМЕНТЫ: %d     БИНТЫ: %d     АПТЕЧКИ: %d"),Medicine,Bandages,Medkits)));
	if(CraftButton)CraftButton->SetIsEnabled(Character->CanCraftMedkit());
	if(UseButton)UseButton->SetIsEnabled(Medkits>0&&Character->Health&&Character->Health->Health<Character->Health->MaxHealth);
}
void UCraftingWidget::CraftMedkitClicked(){if(AShooterCharacter* Character=GetCharacter(this))Character->CraftMedkit();Refresh();}
void UCraftingWidget::UseMedkitClicked(){if(AShooterCharacter* Character=GetCharacter(this))Character->UseMedkit();Refresh();}
void UCraftingWidget::CloseClicked(){if(AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer()))PC->CloseGameplayPanel();}
FReply UCraftingWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::C||E.GetKey()==EKeys::Escape){CloseClicked();return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}

void UStorageTransferSlotWidget::Setup(FName InItemId,int32 InQuantity,bool bInFromChest){ItemId=InItemId;Quantity=InQuantity;bFromChest=bInFromChest;if(Label)Label->SetText(FText::FromString(ItemId.IsNone()?TEXT("ПУСТО"):FString::Printf(TEXT("%s\nx%d"),*ItemId.ToString(),Quantity)));}
void UStorageTransferSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	USizeBox* Size=WidgetTree->ConstructWidget<USizeBox>();WidgetTree->RootWidget=Size;Size->SetWidthOverride(92.f);Size->SetHeightOverride(82.f);
	UBorder* Border=WidgetTree->ConstructWidget<UBorder>();Border->SetBrushColor(ItemId.IsNone()?FLinearColor(.03f,.07f,.09f,.9f):FLinearColor(.08f,.25f,.18f,.98f));Border->SetPadding(FMargin(6.f));Size->AddChild(Border);
	Label=MakeText(WidgetTree,FText::GetEmpty(),13,FLinearColor::White,ETextJustify::Center);Border->AddChild(Label);Setup(ItemId,Quantity,bFromChest);
}
FReply UStorageTransferSlotWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E){if(!ItemId.IsNone()&&E.GetEffectingButton()==EKeys::LeftMouseButton)return UWidgetBlueprintLibrary::DetectDragIfPressed(E,this,EKeys::LeftMouseButton).NativeReply;return Super::NativeOnMouseButtonDown(G,E);}
void UStorageTransferSlotWidget::NativeOnDragDetected(const FGeometry&,const FPointerEvent&,UDragDropOperation*& Operation){if(ItemId.IsNone())return;UItemDragOperation* Drag=NewObject<UItemDragOperation>(this);Drag->ItemId=ItemId;Drag->Quantity=FMath::Max(1,Quantity);Drag->bFromChest=bFromChest;Operation=Drag;}
bool UStorageTransferSlotWidget::NativeOnDrop(const FGeometry&,const FDragDropEvent&,UDragDropOperation* Operation)
{
	UItemDragOperation* Drag=Cast<UItemDragOperation>(Operation);AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer());AShooterCharacter* Character=GetCharacter(this);
	if(!Drag||!PC||!Character||Drag->ItemId.IsNone()||Drag->bFromChest==bFromChest)return false;
	const int32 TransferQuantity=Drag->ItemId.ToString().StartsWith(TEXT("Weapon_"))?1:FMath::Max(1,Drag->Quantity);
	if(Drag->bFromChest)Character->TransferItemFromChest(PC->GetOpenStorageChest(),Drag->ItemId,TransferQuantity);else Character->TransferItemToChest(PC->GetOpenStorageChest(),Drag->ItemId,TransferQuantity);
	return true;
}

void UStorageChestWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;UVerticalBox* Panel=CreatePanel(WidgetTree,FText::FromString(TEXT("СУНДУК — 20 ЯЧЕЕК")),1100.f);
	UHorizontalBox* Columns=WidgetTree->ConstructWidget<UHorizontalBox>();if(UVerticalBoxSlot* ColumnsSlot=Panel->AddChildToVerticalBox(Columns))ColumnsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UVerticalBox* Left=WidgetTree->ConstructWidget<UVerticalBox>();if(UHorizontalBoxSlot* ColumnSlot=Columns->AddChildToHorizontalBox(Left)){ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ColumnSlot->SetPadding(FMargin(5.f,5.f,15.f,5.f));}Left->AddChildToVerticalBox(MakeText(WidgetTree,FText::FromString(TEXT("ВАШ ИНВЕНТАРЬ\nПеретащите стопку в сундук")),17,FLinearColor(.65f,.9f,1.f),ETextJustify::Center));USizeBox* PlayerGridArea=WidgetTree->ConstructWidget<USizeBox>();PlayerGridArea->SetHeightOverride(350.f);PlayerGrid=WidgetTree->ConstructWidget<UUniformGridPanel>();PlayerGridArea->AddChild(PlayerGrid);if(UVerticalBoxSlot* GridSlot=Left->AddChildToVerticalBox(PlayerGridArea))GridSlot->SetPadding(FMargin(2.f,8.f));
	UVerticalBox* Right=WidgetTree->ConstructWidget<UVerticalBox>();if(UHorizontalBoxSlot* ColumnSlot=Columns->AddChildToHorizontalBox(Right)){ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ColumnSlot->SetPadding(FMargin(15.f,5.f,5.f,5.f));}Right->AddChildToVerticalBox(MakeText(WidgetTree,FText::FromString(TEXT("СОДЕРЖИМОЕ СУНДУКА\nПеретащите стопку обратно в инвентарь\nПредметы: 100 в ячейке, оружие: 1")),17,FLinearColor(1.f,.8f,.3f),ETextJustify::Center));USizeBox* ChestGridArea=WidgetTree->ConstructWidget<USizeBox>();ChestGridArea->SetHeightOverride(350.f);ChestGrid=WidgetTree->ConstructWidget<UUniformGridPanel>();ChestGridArea->AddChild(ChestGrid);if(UVerticalBoxSlot* GridSlot=Right->AddChildToVerticalBox(ChestGridArea))GridSlot->SetPadding(FMargin(2.f,8.f));
	StoreWeaponButton=MakeButton(WidgetTree,FText::FromString(TEXT("ПОЛОЖИТЬ ТЕКУЩЕЕ ОРУЖИЕ В СУНДУК")));StoreWeaponButton->OnClicked.AddDynamic(this,&UStorageChestWidget::StoreWeaponClicked);if(UVerticalBoxSlot* WeaponButtonSlot=Left->AddChildToVerticalBox(StoreWeaponButton))WeaponButtonSlot->SetPadding(FMargin(4.f,12.f));
	UButton* Close=MakeButton(WidgetTree,FText::FromString(TEXT("ЗАКРЫТЬ")));Close->OnClicked.AddDynamic(this,&UStorageChestWidget::CloseClicked);Panel->AddChildToVerticalBox(Close);Refresh();
}
void UStorageChestWidget::NativeTick(const FGeometry& G,float Dt){Super::NativeTick(G,Dt);if(GetWorld()&&GetWorld()->GetTimeSeconds()>=NextRefreshTime){NextRefreshTime=GetWorld()->GetTimeSeconds()+.2f;Refresh();}}
void UStorageChestWidget::Refresh()
{
	AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer());AShooterCharacter* Character=GetCharacter(this);AStorageChest* Chest=PC?PC->GetOpenStorageChest():nullptr;if(!Character||!Character->Inventory||!Chest||!Chest->Storage||!PlayerGrid||!ChestGrid)return;
	PlayerGrid->ClearChildren();for(int32 Index=0;Index<Character->Inventory->MaxSlots;++Index){const bool bFilled=Character->Inventory->Items.IsValidIndex(Index);UStorageTransferSlotWidget* Cell=CreateWidget<UStorageTransferSlotWidget>(GetOwningPlayer(),UStorageTransferSlotWidget::StaticClass());Cell->Setup(bFilled?Character->Inventory->Items[Index].ItemId:NAME_None,bFilled?Character->Inventory->Items[Index].Quantity:0,false);PlayerGrid->AddChildToUniformGrid(Cell,Index/5,Index%5);}
	if(StoreWeaponButton)StoreWeaponButton->SetIsEnabled(Character->EquippedWeapon&&Chest->Storage->Items.Num()+Chest->StoredWeapons.Num()<20);
	ChestGrid->ClearChildren();int32 DisplayIndex=0;for(const FInventoryEntry& Entry:Chest->Storage->Items){UStorageTransferSlotWidget* Cell=CreateWidget<UStorageTransferSlotWidget>(GetOwningPlayer(),UStorageTransferSlotWidget::StaticClass());Cell->Setup(Entry.ItemId,Entry.Quantity,true);ChestGrid->AddChildToUniformGrid(Cell,DisplayIndex/5,DisplayIndex%5);++DisplayIndex;}for(AWeaponBase* Weapon:Chest->StoredWeapons){if(!Weapon||DisplayIndex>=20)continue;UStorageTransferSlotWidget* Cell=CreateWidget<UStorageTransferSlotWidget>(GetOwningPlayer(),UStorageTransferSlotWidget::StaticClass());Cell->Setup(Chest->GetStoredWeaponId(Weapon),1,true);ChestGrid->AddChildToUniformGrid(Cell,DisplayIndex/5,DisplayIndex%5);++DisplayIndex;}for(;DisplayIndex<20;++DisplayIndex){UStorageTransferSlotWidget* Cell=CreateWidget<UStorageTransferSlotWidget>(GetOwningPlayer(),UStorageTransferSlotWidget::StaticClass());Cell->Setup(NAME_None,0,true);ChestGrid->AddChildToUniformGrid(Cell,DisplayIndex/5,DisplayIndex%5);}
}
void UStorageChestWidget::StoreWeaponClicked(){AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer());AShooterCharacter* Character=GetCharacter(this);if(PC&&Character)Character->StoreEquippedWeaponInChest(PC->GetOpenStorageChest());}
void UStorageChestWidget::CloseClicked(){if(AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer()))PC->CloseGameplayPanel();}
FReply UStorageChestWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::Escape||E.GetKey()==EKeys::I){CloseClicked();return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}
