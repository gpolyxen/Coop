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
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
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
#include "Engine/Texture2D.h"

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
		UBorder* Border=Tree->ConstructWidget<UBorder>();Border->SetBrushColor(FLinearColor(.16f,.10f,.055f,.98f));Border->SetPadding(FMargin(25.f));Size->AddChild(Border);
		if(UTexture2D* Background=LoadObject<UTexture2D>(nullptr,TEXT("/Game/ThirdPersonBP/UI/Generated/T_InventoryWoodPanel.T_InventoryWoodPanel")))Border->SetBrushFromTexture(Background);
		UVerticalBox* Panel=Tree->ConstructWidget<UVerticalBox>();Border->AddChild(Panel);
		UTextBlock* Header=MakeText(Tree,Title,34,FLinearColor(.65f,.9f,1.f),ETextJustify::Center);
		if(UVerticalBoxSlot* Slot=Panel->AddChildToVerticalBox(Header))Slot->SetPadding(FMargin(5.f,3.f,5.f,15.f));
		return Panel;
	}

	AShooterCharacter* GetCharacter(const UUserWidget* Widget){return Widget?Cast<AShooterCharacter>(Widget->GetOwningPlayerPawn()):nullptr;}
	FString ItemName(FName Id)
	{
		if(Id==TEXT("Wood"))return TEXT("ДЕРЕВО");if(Id==TEXT("Plank"))return TEXT("ДОСКИ");if(Id==TEXT("Stick"))return TEXT("ПАЛКИ");if(Id==TEXT("Rope"))return TEXT("ВЕРЁВКА");
		if(Id==TEXT("Medicine"))return TEXT("МЕДИКАМЕНТЫ");if(Id==TEXT("Bandage"))return TEXT("БИНТЫ");if(Id==TEXT("Medkit"))return TEXT("АПТЕЧКА");
		if(Id==TEXT("Leather"))return TEXT("КОЖА");if(Id==TEXT("Cloth"))return TEXT("ТКАНЬ");if(Id==TEXT("Gasoline"))return TEXT("БЕНЗИН");
		if(Id==TEXT("RifleAmmo"))return TEXT("ПАТРОНЫ");if(Id==TEXT("AK74Ammo"))return TEXT("7,62 ММ");if(Id==TEXT("M4A1Ammo"))return TEXT("5,56 ММ");if(Id==TEXT("Backpack"))return TEXT("РЮКЗАК");if(Id==TEXT("LootBag"))return TEXT("МЕШОК С ПРИПАСАМИ");
		return Id.ToString();
	}
	UTexture2D* ItemIcon(FName Id)
	{
		if(Id==TEXT("Wood"))return LoadObject<UTexture2D>(nullptr,TEXT("/Game/ThirdPersonBP/UI/Generated/T_ItemWood.T_ItemWood"));
		if(Id==TEXT("Plank"))return LoadObject<UTexture2D>(nullptr,TEXT("/Game/ThirdPersonBP/UI/Generated/T_ItemPlank.T_ItemPlank"));
		if(Id==TEXT("Rope"))return LoadObject<UTexture2D>(nullptr,TEXT("/Game/ThirdPersonBP/UI/Generated/T_ItemRope.T_ItemRope"));
		if(Id==TEXT("AK74Ammo"))return LoadObject<UTexture2D>(nullptr,TEXT("/Game/ThirdPersonBP/Items/7_62/7_62.7_62"));
		if(Id==TEXT("M4A1Ammo"))return LoadObject<UTexture2D>(nullptr,TEXT("/Game/ThirdPersonBP/Items/5_56_NATO/5_56NATO.5_56NATO"));
		if(Id==TEXT("Medkit"))return LoadObject<UTexture2D>(nullptr,TEXT("/Game/ThirdPersonBP/Items/Small_Medkit/small_medkit.small_medkit"));
		return nullptr;
	}
}

void UInventoryItemSlotWidget::Setup(FName InItemId,int32 InQuantity,int32 InSlotIndex){ItemId=InItemId;Quantity=InQuantity;SlotIndex=InSlotIndex;RefreshLabel();}
void UInventoryItemSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	UBorder* Border=WidgetTree->ConstructWidget<UBorder>();WidgetTree->RootWidget=Border;Border->SetBrushColor(FLinearColor(0.f,0.f,0.f,0.f));Border->SetPadding(FMargin(0.f));
	USizeBox* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(66.f);Size->SetHeightOverride(67.f);Border->AddChild(Size);
	UOverlay* Contents=WidgetTree->ConstructWidget<UOverlay>();Size->AddChild(Contents);
	Icon=WidgetTree->ConstructWidget<UImage>();Icon->SetOpacity(.9f);Icon->SetVisibility(ESlateVisibility::Collapsed);
	if(UOverlaySlot* IconSlot=Contents->AddChildToOverlay(Icon)){IconSlot->SetPadding(FMargin(4.f));IconSlot->SetHorizontalAlignment(HAlign_Fill);IconSlot->SetVerticalAlignment(VAlign_Fill);}
	Label=MakeText(WidgetTree,FText::GetEmpty(),15,FLinearColor::White,ETextJustify::Center);Label->SetVisibility(ESlateVisibility::HitTestInvisible);Label->SetShadowOffset(FVector2D(2.f,2.f));Label->SetShadowColorAndOpacity(FLinearColor::Black);
	if(UOverlaySlot* LabelSlot=Contents->AddChildToOverlay(Label)){LabelSlot->SetHorizontalAlignment(HAlign_Fill);LabelSlot->SetVerticalAlignment(VAlign_Bottom);LabelSlot->SetPadding(FMargin(3.f));}RefreshLabel();
}
void UInventoryItemSlotWidget::RefreshLabel(){if(!Label)return;UTexture2D* Texture=ItemIcon(ItemId);if(Icon){Icon->SetVisibility(Texture?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);if(Texture)Icon->SetBrushFromTexture(Texture,true);}const FString Selection=SelectedQuantity>0?FString::Printf(TEXT(" [%d]"),SelectedQuantity):TEXT("");if(ItemId.IsNone())Label->SetText(FText::GetEmpty());else if(Texture)Label->SetText(FText::FromString(FString::Printf(TEXT("%d%s"),Quantity,*Selection)));else Label->SetText(FText::FromString(FString::Printf(TEXT("%s\n%d%s"),*ItemName(ItemId),Quantity,*Selection)));}
FReply UInventoryItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E){if(ItemId==TEXT("Medkit")&&E.GetEffectingButton()==EKeys::LeftMouseButton&&GetWorld()){const float Now=GetWorld()->GetRealTimeSeconds();if(Now-LastLeftClickTime<=.4f){if(AShooterCharacter* Character=GetCharacter(this))Character->UseMedkit();LastLeftClickTime=-100.f;return FReply::Handled();}LastLeftClickTime=Now;}if(!ItemId.IsNone()&&E.GetEffectingButton()==EKeys::LeftMouseButton){SelectedQuantity=FMath::Clamp(SelectedQuantity+(E.IsShiftDown()?10:1),1,Quantity);RefreshLabel();return UWidgetBlueprintLibrary::DetectDragIfPressed(E,this,EKeys::LeftMouseButton).NativeReply;}return Super::NativeOnMouseButtonDown(G,E);}
FReply UInventoryItemSlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& G,const FPointerEvent& E){if(ItemId==TEXT("Medkit")&&E.GetEffectingButton()==EKeys::LeftMouseButton){if(AShooterCharacter* Character=GetCharacter(this))Character->UseMedkit();return FReply::Handled();}return Super::NativeOnMouseButtonDoubleClick(G,E);}
void UInventoryItemSlotWidget::NativeOnDragDetected(const FGeometry&,const FPointerEvent&,UDragDropOperation*& Operation){if(ItemId.IsNone())return;const int32 TransferQuantity=FMath::Clamp(SelectedQuantity,1,Quantity);UItemDragOperation* Drag=NewObject<UItemDragOperation>(this);Drag->ItemId=ItemId;Drag->Quantity=TransferQuantity;Drag->bFromChest=false;UTextBlock* Visual=NewObject<UTextBlock>(Drag);Visual->SetText(FText::FromString(FString::Printf(TEXT("%s  x%d"),*ItemName(ItemId),TransferQuantity)));Visual->SetColorAndOpacity(FSlateColor(FLinearColor(1.f,.85f,.35f)));Visual->SetShadowOffset(FVector2D(2.f,2.f));Visual->SetShadowColorAndOpacity(FLinearColor::Black);Visual->SetVisibility(ESlateVisibility::HitTestInvisible);Drag->DefaultDragVisual=Visual;Drag->Pivot=EDragPivot::CenterCenter;Operation=Drag;}

void UCraftingIngredientSlotWidget::Setup(UInventoryWidget* InOwner,int32 InIndex){OwnerWidget=InOwner;Index=InIndex;RefreshLabel();}
void UCraftingIngredientSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	UBorder* Border=WidgetTree->ConstructWidget<UBorder>();WidgetTree->RootWidget=Border;Border->SetBrushColor(FLinearColor(0.f,0.f,0.f,0.f));Border->SetPadding(FMargin(0.f));
	USizeBox* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(54.f);Size->SetHeightOverride(55.f);Border->AddChild(Size);
	UOverlay* Contents=WidgetTree->ConstructWidget<UOverlay>();Size->AddChild(Contents);Icon=WidgetTree->ConstructWidget<UImage>();Icon->SetVisibility(ESlateVisibility::Collapsed);if(UOverlaySlot* S=Contents->AddChildToOverlay(Icon)){S->SetPadding(FMargin(2.f));S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Fill);}Label=MakeText(WidgetTree,FText::GetEmpty(),11,FLinearColor(1.f,.85f,.35f),ETextJustify::Center);Label->SetVisibility(ESlateVisibility::HitTestInvisible);Label->SetShadowOffset(FVector2D(2.f,2.f));if(UOverlaySlot* S=Contents->AddChildToOverlay(Label)){S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Bottom);}RefreshLabel();
}
void UCraftingIngredientSlotWidget::SetIngredient(FName InItemId,int32 InQuantity){ItemId=InItemId;Quantity=InQuantity;RefreshLabel();}
void UCraftingIngredientSlotWidget::RefreshLabel(){if(Icon){UTexture2D* T=ItemIcon(ItemId);Icon->SetVisibility(T?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);if(T)Icon->SetBrushFromTexture(T,true);}if(Label)Label->SetText(ItemId.IsNone()?FText::GetEmpty():FText::FromString(FString::Printf(TEXT("%d"),Quantity)));}
bool UCraftingIngredientSlotWidget::NativeOnDrop(const FGeometry&,const FDragDropEvent&,UDragDropOperation* Operation){if(UItemDragOperation* Drag=Cast<UItemDragOperation>(Operation)){if(OwnerWidget)OwnerWidget->SetCraftIngredient(Index,Drag->ItemId,Drag->Quantity);return true;}return false;}
FReply UCraftingIngredientSlotWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E){if(E.GetEffectingButton()==EKeys::RightMouseButton){if(OwnerWidget)OwnerWidget->SetCraftIngredient(Index,NAME_None,0);return FReply::Handled();}return Super::NativeOnMouseButtonDown(G,E);}

void UInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	UOverlay* Root=WidgetTree->ConstructWidget<UOverlay>();WidgetTree->RootWidget=Root;
	UBorder* Dim=WidgetTree->ConstructWidget<UBorder>();Dim->SetBrushColor(FLinearColor(0.f,0.f,0.f,.58f));if(UOverlaySlot* S=Root->AddChildToOverlay(Dim)){S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Fill);}
	USizeBox* Frame=WidgetTree->ConstructWidget<USizeBox>();Frame->SetWidthOverride(1280.f);Frame->SetHeightOverride(720.f);if(UOverlaySlot* S=Root->AddChildToOverlay(Frame)){S->SetHorizontalAlignment(HAlign_Center);S->SetVerticalAlignment(VAlign_Center);}
	UBorder* Background=WidgetTree->ConstructWidget<UBorder>();Background->SetPadding(FMargin(0.f));if(UTexture2D* T=LoadObject<UTexture2D>(nullptr,TEXT("/Game/ThirdPersonBP/UI/Generated/T_InventoryWoodPanel.T_InventoryWoodPanel")))Background->SetBrushFromTexture(T);Frame->AddChild(Background);
	UCanvasPanel* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Background->AddChild(Canvas);
	auto Place=[&](UWidget* Widget,float X,float Y,float W,float H){if(UCanvasPanelSlot* S=Canvas->AddChildToCanvas(Widget)){S->SetPosition(FVector2D(X,Y));S->SetSize(FVector2D(W,H));}};
	Place(MakeText(WidgetTree,FText::FromString(TEXT("ИНВЕНТАРЬ [I]")),28,FLinearColor(.8f,.93f,1.f),ETextJustify::Center),35.f,35.f,545.f,42.f);
	CapacityText=MakeText(WidgetTree,FText::GetEmpty(),17,FLinearColor(.55f,.85f,1.f),ETextJustify::Center);Place(CapacityText,35.f,75.f,545.f,30.f);
	InventoryGrid=WidgetTree->ConstructWidget<UUniformGridPanel>();InventoryGrid->SetMinDesiredSlotWidth(68.5f);InventoryGrid->SetMinDesiredSlotHeight(70.f);Place(InventoryGrid,32.f,111.f,550.f,560.f);
	UVerticalBox* CharacterPanel=WidgetTree->ConstructWidget<UVerticalBox>();Place(CharacterPanel,612.f,32.f,365.f,455.f);
	CharacterPanel->AddChildToVerticalBox(MakeText(WidgetTree,FText::FromString(TEXT("ПЕРСОНАЖ")),19,FLinearColor(1.f,.82f,.45f),ETextJustify::Center));
	USizeBox* Preview=WidgetTree->ConstructWidget<USizeBox>();Preview->SetHeightOverride(320.f);UBorder* PreviewBorder=WidgetTree->ConstructWidget<UBorder>();PreviewBorder->SetBrushColor(FLinearColor(.025f,.018f,.012f,.45f));PreviewBorder->SetPadding(FMargin(12.f));Preview->AddChild(PreviewBorder);PreviewBorder->AddChild(MakeText(WidgetTree,FText::FromString(TEXT("МЕСТО ДЛЯ МОДЕЛИ\nПЕРСОНАЖА")),15,FLinearColor(.62f,.58f,.5f),ETextJustify::Center));CharacterPanel->AddChildToVerticalBox(Preview);
	CharacterPanel->AddChildToVerticalBox(MakeText(WidgetTree,FText::FromString(TEXT("ОРУЖИЕ — 2 АКТИВНЫХ СЛОТА")),15,FLinearColor(.9f,.75f,.42f),ETextJustify::Center));
	UUniformGridPanel* WeaponSlots=WidgetTree->ConstructWidget<UUniformGridPanel>();for(int32 Index=0;Index<2;++Index){USizeBox* Cell=WidgetTree->ConstructWidget<USizeBox>();Cell->SetWidthOverride(170.f);Cell->SetHeightOverride(58.f);UBorder* CellBorder=WidgetTree->ConstructWidget<UBorder>();CellBorder->SetBrushColor(FLinearColor(.08f,.055f,.03f,.62f));CellBorder->SetPadding(FMargin(5.f));Cell->AddChild(CellBorder);CellBorder->AddChild(MakeText(WidgetTree,FText::FromString(FString::Printf(TEXT("ОРУЖИЕ %d"),Index+1)),13,FLinearColor::White,ETextJustify::Center));WeaponSlots->AddChildToUniformGrid(Cell,0,Index);}CharacterPanel->AddChildToVerticalBox(WeaponSlots);
	Place(MakeText(WidgetTree,FText::FromString(TEXT("КРАФТ — 8 ЯЧЕЕК  [ENTER]")),17,FLinearColor(1.f,.78f,.25f),ETextJustify::Center),1015.f,118.f,245.f,35.f);
	CraftGrid=WidgetTree->ConstructWidget<UUniformGridPanel>();CraftGrid->SetMinDesiredSlotWidth(57.f);CraftGrid->SetMinDesiredSlotHeight(57.f);Place(CraftGrid,1018.f,166.f,228.f,114.f);
	CraftIngredients.SetNum(8);CraftQuantities.Init(0,8);for(int32 Index=0;Index<8;++Index){UCraftingIngredientSlotWidget* SlotWidget=CreateWidget<UCraftingIngredientSlotWidget>(GetOwningPlayer(),UCraftingIngredientSlotWidget::StaticClass());SlotWidget->Setup(this,Index);CraftSlots.Add(SlotWidget);CraftGrid->AddChildToUniformGrid(SlotWidget,Index/4,Index%4);}
	RecipeList=WidgetTree->ConstructWidget<UVerticalBox>();Place(RecipeList,1015.f,305.f,245.f,85.f);
	CraftSelectedButton=nullptr;
	Refresh();
}

void UInventoryWidget::NativeTick(const FGeometry& MyGeometry,float InDeltaTime)
{
	Super::NativeTick(MyGeometry,InDeltaTime);
	if(GetWorld()&&GetWorld()->GetTimeSeconds()>=NextRefreshTime){NextRefreshTime=GetWorld()->GetTimeSeconds()+.5f;if(CalculateInventoryHash()!=LastInventoryHash)Refresh();}
}

int32 UInventoryWidget::CalculateInventoryHash()const
{
	const AShooterCharacter* Character=GetCharacter(this);if(!Character||!Character->Inventory)return 0;
	uint32 Hash=GetTypeHash(Character->Inventory->Items.Num());
	for(const FInventoryEntry& Entry:Character->Inventory->Items){Hash=HashCombine(Hash,GetTypeHash(Entry.ItemId));Hash=HashCombine(Hash,GetTypeHash(Entry.Quantity));}
	return static_cast<int32>(Hash);
}

void UInventoryWidget::Refresh()
{
	AShooterCharacter* Character=GetCharacter(this);if(!Character||!Character->Inventory||!CapacityText||!InventoryGrid)return;
	UInventoryComponent* Inventory=Character->Inventory;
	LastInventoryHash=CalculateInventoryHash();
	CapacityText->SetText(FText::FromString(FString::Printf(TEXT("СЛОТЫ: %d / %d     МАКС. ВЕС: %.0f КГ"),Inventory->Items.Num(),Inventory->MaxSlots,Inventory->MaxWeight)));
	InventoryGrid->ClearChildren();
	TMap<FName,int32> Reserved;for(int32 Index=0;Index<CraftIngredients.Num();++Index)if(!CraftIngredients[Index].IsNone()&&CraftQuantities.IsValidIndex(Index))Reserved.FindOrAdd(CraftIngredients[Index])+=CraftQuantities[Index];
	// Always create the complete 8x8 visual grid painted into the background.
	// UniformGrid otherwise divides
	// its width by 6 when capacity is six, then suddenly changes to eight columns
	// as soon as a second row is present, stretching the first row across the art.
	const int32 DisplaySlotCount=FMath::Max(64,Inventory->MaxSlots);
	for(int32 SlotIndex=0;SlotIndex<DisplaySlotCount;++SlotIndex)
	{
		const FName Item=Inventory->Items.IsValidIndex(SlotIndex)?Inventory->Items[SlotIndex].ItemId:NAME_None;int32 Quantity=Inventory->Items.IsValidIndex(SlotIndex)?Inventory->Items[SlotIndex].Quantity:0;if(int32* Pending=Reserved.Find(Item)){const int32 Taken=FMath::Min(Quantity,*Pending);Quantity-=Taken;*Pending-=Taken;}
		UInventoryItemSlotWidget* Cell=CreateWidget<UInventoryItemSlotWidget>(GetOwningPlayer(),UInventoryItemSlotWidget::StaticClass());Cell->Setup(Item,Quantity,SlotIndex);
		if(SlotIndex>=Inventory->MaxSlots){Cell->SetRenderOpacity(.24f);Cell->SetVisibility(ESlateVisibility::HitTestInvisible);}
		InventoryGrid->AddChildToUniformGrid(Cell,SlotIndex/8,SlotIndex%8);
	}
	int32 Medicine=0,Bandage=0,Wood=0;for(int32 Index=0;Index<CraftIngredients.Num();++Index){const int32 Q=CraftQuantities.IsValidIndex(Index)?CraftQuantities[Index]:0;if(CraftIngredients[Index]==TEXT("Medicine"))Medicine+=Q;else if(CraftIngredients[Index]==TEXT("Bandage"))Bandage+=Q;else if(CraftIngredients[Index]==TEXT("Wood"))Wood+=Q;}
	int32 Plank=0;for(int32 Index=0;Index<CraftIngredients.Num();++Index)if(CraftIngredients[Index]==TEXT("Plank"))Plank+=CraftQuantities.IsValidIndex(Index)?CraftQuantities[Index]:0;
	const bool bMedkitRecipe=Medicine>=1&&Bandage>=2,bPlankRecipe=Wood>=1,bStickRecipe=Plank>=1;if(RecipeList){RecipeList->ClearChildren();FString Recipes;if(bMedkitRecipe)Recipes+=TEXT("АПТЕЧКА — 1 медикамент + 2 бинта");if(bPlankRecipe){if(!Recipes.IsEmpty())Recipes+=TEXT("\n");Recipes+=FString::Printf(TEXT("%d дерева → %d досок  [ENTER]"),Wood,Wood*4);}if(bStickRecipe){if(!Recipes.IsEmpty())Recipes+=TEXT("\n");Recipes+=FString::Printf(TEXT("%d досок → %d палок  [ENTER]"),Plank,Plank*4);}RecipeList->AddChildToVerticalBox(MakeText(WidgetTree,FText::FromString(Recipes.IsEmpty()?TEXT("ДОСТУПНЫХ РЕЦЕПТОВ НЕТ"):FString(TEXT("ДОСТУПНО:\n"))+Recipes),15,Recipes.IsEmpty()?FLinearColor(.55f,.55f,.55f):FLinearColor(.35f,1.f,.45f)));}
	if(CraftSelectedButton)CraftSelectedButton->SetIsEnabled((bMedkitRecipe&&Character->CanCraftMedkit())||(bPlankRecipe&&Character->CanCraftPlanks())||(bStickRecipe&&Character->CanCraftSticks()));
}

void UInventoryWidget::SetCraftIngredient(int32 Index,FName ItemId,int32 Quantity){if(!CraftIngredients.IsValidIndex(Index)||!CraftQuantities.IsValidIndex(Index))return;if(ItemId.IsNone()){CraftIngredients[Index]=NAME_None;CraftQuantities[Index]=0;}else if(CraftIngredients[Index]==ItemId)CraftQuantities[Index]=FMath::Clamp(CraftQuantities[Index]+FMath::Max(1,Quantity),1,999);else{CraftIngredients[Index]=ItemId;CraftQuantities[Index]=FMath::Max(1,Quantity);}if(CraftSlots.IsValidIndex(Index)&&CraftSlots[Index])CraftSlots[Index]->SetIngredient(CraftIngredients[Index],CraftQuantities[Index]);Refresh();}
void UInventoryWidget::CraftSelectedClicked(){AShooterCharacter* Character=GetCharacter(this);if(!Character)return;int32 Medicine=0,Bandage=0,Wood=0,Plank=0;for(int32 Index=0;Index<CraftIngredients.Num();++Index){const int32 Q=CraftQuantities.IsValidIndex(Index)?CraftQuantities[Index]:0;if(CraftIngredients[Index]==TEXT("Medicine"))Medicine+=Q;else if(CraftIngredients[Index]==TEXT("Bandage"))Bandage+=Q;else if(CraftIngredients[Index]==TEXT("Wood"))Wood+=Q;else if(CraftIngredients[Index]==TEXT("Plank"))Plank+=Q;}if(Medicine>=1&&Bandage>=2)Character->CraftMedkit();else if(Wood>=1)Character->CraftPlankBatches(Wood);else if(Plank>=1)Character->CraftStickBatches(Plank);else return;for(int32 Index=0;Index<CraftIngredients.Num();++Index){CraftIngredients[Index]=NAME_None;CraftQuantities[Index]=0;if(CraftSlots.IsValidIndex(Index)&&CraftSlots[Index])CraftSlots[Index]->SetIngredient(NAME_None,0);}Refresh();}

void UInventoryWidget::CloseClicked(){if(AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer()))PC->CloseGameplayPanel();}
FReply UInventoryWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry,const FKeyEvent& Event){if(Event.GetKey()==EKeys::Enter){CraftSelectedClicked();return FReply::Handled();}return Super::NativeOnPreviewKeyDown(InGeometry,Event);}
FReply UInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& Event){if(Event.GetKey()==EKeys::I||Event.GetKey()==EKeys::Escape){CloseClicked();return FReply::Handled();}if(Event.GetKey()==EKeys::Enter){CraftSelectedClicked();return FReply::Handled();}return Super::NativeOnKeyDown(InGeometry,Event);}

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
void UBuildingMenuWidget::ShowFurniture(){Content->ClearChildren();UButton* Bed=MakeButton(WidgetTree,FText::FromString(TEXT("КРОВАТЬ\n10 досок + 10 верёвок\nСохранение игры")));Bed->OnClicked.AddDynamic(this,&UBuildingMenuWidget::BedClicked);Content->AddChildToVerticalBox(Bed);UButton* Chest=MakeButton(WidgetTree,FText::FromString(TEXT("СУНДУК\n20 ячеек | 20 досок + 5 кожи")));Chest->OnClicked.AddDynamic(this,&UBuildingMenuWidget::ChestClicked);Content->AddChildToVerticalBox(Chest);UButton* Back=MakeButton(WidgetTree,FText::FromString(TEXT("НАЗАД К РАЗДЕЛАМ")));Back->OnClicked.AddDynamic(this,&UBuildingMenuWidget::BackClicked);Content->AddChildToVerticalBox(Back);}
void UBuildingMenuWidget::ShowDefense(){Content->ClearChildren();UButton* Wall=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННАЯ СТЕНА\n6 досок | 10 ударов топором | секция 3 м")));Wall->OnClicked.AddDynamic(this,&UBuildingMenuWidget::WallClicked);Content->AddChildToVerticalBox(Wall);UButton* WindowWall=MakeButton(WidgetTree,FText::FromString(TEXT("СТЕНА С ОКНОМ\n7 досок | 10 ударов топором | секция 3 м")));WindowWall->OnClicked.AddDynamic(this,&UBuildingMenuWidget::WindowWallClicked);Content->AddChildToVerticalBox(WindowWall);UButton* Door=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННАЯ ДВЕРЬ\n8 досок + 2 верёвки | 10 ударов топором")));Door->OnClicked.AddDynamic(this,&UBuildingMenuWidget::DoorClicked);Content->AddChildToVerticalBox(Door);UButton* Gate=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННЫЕ ВОРОТА\n12 досок + 4 верёвки | 10 ударов топором")));Gate->OnClicked.AddDynamic(this,&UBuildingMenuWidget::GateClicked);Content->AddChildToVerticalBox(Gate);UButton* Floor=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННЫЙ ПОЛ\n5 досок | площадка 3 x 3 м\nДо 3 секций от несущей опоры")));Floor->OnClicked.AddDynamic(this,&UBuildingMenuWidget::FloorClicked);Content->AddChildToVerticalBox(Floor);UButton* Stairs=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННАЯ ЛЕСТНИЦА\n8 досок | подъём на один этаж")));Stairs->OnClicked.AddDynamic(this,&UBuildingMenuWidget::StairsClicked);Content->AddChildToVerticalBox(Stairs);UButton* Pillar=MakeButton(WidgetTree,FText::FromString(TEXT("ДЕРЕВЯННАЯ КОЛОННА\n4 доски | опора для пола второго этажа")));Pillar->OnClicked.AddDynamic(this,&UBuildingMenuWidget::PillarClicked);Content->AddChildToVerticalBox(Pillar);UButton* Back=MakeButton(WidgetTree,FText::FromString(TEXT("НАЗАД К РАЗДЕЛАМ")));Back->OnClicked.AddDynamic(this,&UBuildingMenuWidget::BackClicked);Content->AddChildToVerticalBox(Back);}
void UBuildingMenuWidget::ShowLighting(){Content->ClearChildren();UButton* Torch=MakeButton(WidgetTree,FText::FromString(TEXT("НАСТЕННЫЙ ФАКЕЛ\n1 палка + 1 ткань + 1 бензин\nАвтоматически загорается ночью")));Torch->OnClicked.AddDynamic(this,&UBuildingMenuWidget::TorchClicked);Content->AddChildToVerticalBox(Torch);UButton* Back=MakeButton(WidgetTree,FText::FromString(TEXT("НАЗАД К РАЗДЕЛАМ")));Back->OnClicked.AddDynamic(this,&UBuildingMenuWidget::BackClicked);Content->AddChildToVerticalBox(Back);}
void UBuildingMenuWidget::SelectBuildPiece(EBuildPieceType Type){if(AShooterCharacter* Character=GetCharacter(this))Character->BeginBuildPlacement(Type);CloseClicked();}
void UBuildingMenuWidget::FurnitureClicked(){ShowFurniture();}void UBuildingMenuWidget::DefenseClicked(){ShowDefense();}void UBuildingMenuWidget::LightingClicked(){ShowLighting();}void UBuildingMenuWidget::BackClicked(){ShowCategories();}
void UBuildingMenuWidget::BedClicked(){SelectBuildPiece(EBuildPieceType::Bed);}void UBuildingMenuWidget::ChestClicked(){SelectBuildPiece(EBuildPieceType::WoodChest);}void UBuildingMenuWidget::WallClicked(){SelectBuildPiece(EBuildPieceType::WoodWall);}void UBuildingMenuWidget::WindowWallClicked(){SelectBuildPiece(EBuildPieceType::WoodWindowWall);}void UBuildingMenuWidget::DoorClicked(){SelectBuildPiece(EBuildPieceType::WoodDoor);}void UBuildingMenuWidget::GateClicked(){SelectBuildPiece(EBuildPieceType::WoodGate);}void UBuildingMenuWidget::FloorClicked(){SelectBuildPiece(EBuildPieceType::WoodFloor);}void UBuildingMenuWidget::StairsClicked(){SelectBuildPiece(EBuildPieceType::WoodStairs);}void UBuildingMenuWidget::PillarClicked(){SelectBuildPiece(EBuildPieceType::WoodPillar);}void UBuildingMenuWidget::TorchClicked(){SelectBuildPiece(EBuildPieceType::WallTorch);}
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
	CraftPlanksButton=MakeButton(WidgetTree,FText::FromString(TEXT("ДОСКИ x4\n1 дерево\nОсновной строительный материал")));CraftPlanksButton->OnClicked.AddDynamic(this,&UCraftingWidget::CraftPlanksClicked);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(CraftPlanksButton))PanelSlot->SetPadding(FMargin(4.f,5.f));
	CraftSticksButton=MakeButton(WidgetTree,FText::FromString(TEXT("ПАЛКИ x4\n1 доска\nДля факелов и других рецептов")));CraftSticksButton->OnClicked.AddDynamic(this,&UCraftingWidget::CraftSticksClicked);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(CraftSticksButton))PanelSlot->SetPadding(FMargin(4.f,5.f));
	UseButton=MakeButton(WidgetTree,FText::FromString(TEXT("ИСПОЛЬЗОВАТЬ АПТЕЧКУ")));UseButton->OnClicked.AddDynamic(this,&UCraftingWidget::UseMedkitClicked);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(UseButton))PanelSlot->SetPadding(FMargin(4.f,12.f,4.f,5.f));
	UButton* Close=MakeButton(WidgetTree,FText::FromString(TEXT("ЗАКРЫТЬ")));Close->OnClicked.AddDynamic(this,&UCraftingWidget::CloseClicked);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(Close))PanelSlot->SetPadding(FMargin(4.f,18.f,4.f,2.f));Refresh();
}
void UCraftingWidget::NativeTick(const FGeometry& G,float Dt){Super::NativeTick(G,Dt);if(GetWorld()&&GetWorld()->GetTimeSeconds()>=NextRefreshTime){NextRefreshTime=GetWorld()->GetTimeSeconds()+.2f;Refresh();}}
void UCraftingWidget::Refresh()
{
	AShooterCharacter* Character=GetCharacter(this);if(!Character||!Character->Inventory)return;
	const int32 Medicine=Character->Inventory->GetQuantity(TEXT("Medicine")),Bandages=Character->Inventory->GetQuantity(TEXT("Bandage")),Medkits=Character->Inventory->GetQuantity(TEXT("Medkit")),Wood=Character->Inventory->GetQuantity(TEXT("Wood")),Planks=Character->Inventory->GetQuantity(TEXT("Plank")),Sticks=Character->Inventory->GetQuantity(TEXT("Stick"));
	if(MaterialsText)MaterialsText->SetText(FText::FromString(FString::Printf(TEXT("ДЕРЕВО: %d     ДОСКИ: %d     ПАЛКИ: %d\nМЕДИКАМЕНТЫ: %d     БИНТЫ: %d     АПТЕЧКИ: %d"),Wood,Planks,Sticks,Medicine,Bandages,Medkits)));
	if(CraftButton)CraftButton->SetIsEnabled(Character->CanCraftMedkit());
	if(CraftPlanksButton)CraftPlanksButton->SetIsEnabled(Character->CanCraftPlanks());
	if(CraftSticksButton)CraftSticksButton->SetIsEnabled(Character->CanCraftSticks());
	if(UseButton)UseButton->SetIsEnabled(Medkits>0&&Character->Health&&Character->Health->Health<Character->Health->MaxHealth);
}
void UCraftingWidget::CraftMedkitClicked(){if(AShooterCharacter* Character=GetCharacter(this))Character->CraftMedkit();Refresh();}
void UCraftingWidget::CraftPlanksClicked(){if(AShooterCharacter* Character=GetCharacter(this))Character->CraftPlankBatches(1);Refresh();}
void UCraftingWidget::CraftSticksClicked(){if(AShooterCharacter* Character=GetCharacter(this))Character->CraftSticks();Refresh();}
void UCraftingWidget::UseMedkitClicked(){if(AShooterCharacter* Character=GetCharacter(this))Character->UseMedkit();Refresh();}
void UCraftingWidget::CloseClicked(){if(AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer()))PC->CloseGameplayPanel();}
FReply UCraftingWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::C||E.GetKey()==EKeys::Escape){CloseClicked();return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}

void UStorageTransferSlotWidget::Setup(FName InItemId,int32 InQuantity,bool bInFromChest){ItemId=InItemId;Quantity=InQuantity;bFromChest=bInFromChest;if(Label)Label->SetText(FText::FromString(ItemId.IsNone()?TEXT("ПУСТО"):FString::Printf(TEXT("%s\nx%d"),*ItemName(ItemId),Quantity)));}
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
	UButton* TakeAll=MakeButton(WidgetTree,FText::FromString(TEXT("ЗАБРАТЬ ВСЁ  [T]")));TakeAll->OnClicked.AddDynamic(this,&UStorageChestWidget::TakeAllClicked);if(UVerticalBoxSlot* TakeAllSlot=Right->AddChildToVerticalBox(TakeAll))TakeAllSlot->SetPadding(FMargin(4.f,12.f));
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
void UStorageChestWidget::TakeAllClicked(){AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer());AShooterCharacter* Character=GetCharacter(this);if(PC&&Character)Character->TakeAllFromChest(PC->GetOpenStorageChest());}
void UStorageChestWidget::CloseClicked(){if(AShooterPlayerController* PC=Cast<AShooterPlayerController>(GetOwningPlayer()))PC->CloseGameplayPanel();}
FReply UStorageChestWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::T){TakeAllClicked();return FReply::Handled();}if(E.GetKey()==EKeys::Escape||E.GetKey()==EKeys::I){CloseClicked();return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}
