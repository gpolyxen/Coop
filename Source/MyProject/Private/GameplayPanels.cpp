#include "GameplayPanels.h"

#include "InventoryComponent.h"
#include "ShooterCharacter.h"
#include "ShooterPlayerController.h"
#include "WeaponBase.h"
#include "Blueprint/WidgetTree.h"
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

void UInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();bIsFocusable=true;
	UVerticalBox* Panel=CreatePanel(WidgetTree,FText::FromString(TEXT("ИНВЕНТАРЬ  [I]")),650.f);
	CapacityText=MakeText(WidgetTree,FText::GetEmpty(),20,FLinearColor(.55f,.85f,1.f),ETextJustify::Center);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(CapacityText))PanelSlot->SetPadding(FMargin(4.f,2.f,4.f,10.f));
	UScrollBox* Scroll=WidgetTree->ConstructWidget<UScrollBox>();
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(Scroll))PanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ItemList=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(ItemList);
	UButton* Close=MakeButton(WidgetTree,FText::FromString(TEXT("ЗАКРЫТЬ")));Close->OnClicked.AddDynamic(this,&UInventoryWidget::CloseClicked);
	if(UVerticalBoxSlot* PanelSlot=Panel->AddChildToVerticalBox(Close))PanelSlot->SetPadding(FMargin(4.f,14.f,4.f,2.f));
	Refresh();
}

void UInventoryWidget::NativeTick(const FGeometry& MyGeometry,float InDeltaTime)
{
	Super::NativeTick(MyGeometry,InDeltaTime);
	if(GetWorld()&&GetWorld()->GetTimeSeconds()>=NextRefreshTime){NextRefreshTime=GetWorld()->GetTimeSeconds()+.25f;Refresh();}
}

void UInventoryWidget::Refresh()
{
	AShooterCharacter* Character=GetCharacter(this);if(!Character||!Character->Inventory||!CapacityText||!ItemList)return;
	UInventoryComponent* Inventory=Character->Inventory;
	CapacityText->SetText(FText::FromString(FString::Printf(TEXT("СЛОТЫ: %d / %d     МАКС. ВЕС: %.0f КГ"),Inventory->Items.Num(),Inventory->MaxSlots,Inventory->MaxWeight)));
	ItemList->ClearChildren();
	for(int32 SlotIndex=0;SlotIndex<Inventory->MaxSlots;++SlotIndex)
	{
		FString Line=FString::Printf(TEXT("%02d.  — пусто —"),SlotIndex+1);
		if(Inventory->Items.IsValidIndex(SlotIndex))Line=FString::Printf(TEXT("%02d.  %s   x%d"),SlotIndex+1,*Inventory->Items[SlotIndex].ItemId.ToString(),Inventory->Items[SlotIndex].Quantity);
		UTextBlock* Row=MakeText(WidgetTree,FText::FromString(Line),18,Inventory->Items.IsValidIndex(SlotIndex)?FLinearColor::White:FLinearColor(.35f,.45f,.5f));
		if(UVerticalBoxSlot* PanelSlot=ItemList->AddChildToVerticalBox(Row))PanelSlot->SetPadding(FMargin(12.f,5.f));
	}
	if(Character->WeaponSlots.Num()>0)
	{
		UTextBlock* Weapons=MakeText(WidgetTree,FText::FromString(TEXT("ОРУЖИЕ")),21,FLinearColor(.95f,.7f,.2f));
		if(UVerticalBoxSlot* PanelSlot=ItemList->AddChildToVerticalBox(Weapons))PanelSlot->SetPadding(FMargin(12.f,15.f,12.f,4.f));
		for(AWeaponBase* Weapon:Character->WeaponSlots)if(Weapon)
		{
			UTextBlock* Row=MakeText(WidgetTree,FText::FromString(FString::Printf(TEXT("• %s   %d / %d"),*Weapon->WeaponName,Weapon->AmmoInMagazine,Weapon->ReserveAmmo)),18,FLinearColor::White);
			if(UVerticalBoxSlot* PanelSlot=ItemList->AddChildToVerticalBox(Row))PanelSlot->SetPadding(FMargin(12.f,4.f));
		}
	}
}

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
