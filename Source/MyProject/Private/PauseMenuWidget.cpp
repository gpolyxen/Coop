#include "PauseMenuWidget.h"

#include "ShooterGameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

namespace
{
	UTextBlock* AddPauseText(UWidgetTree* Tree,UVerticalBox* Parent,const FText& Text,int32 Size,const FLinearColor& Color)
	{
		UTextBlock* Label=Tree->ConstructWidget<UTextBlock>();
		Label->SetText(Text);
		Label->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo Font=Label->Font;
		Font.Size=Size;
		Label->SetFont(Font);
		Label->SetJustification(ETextJustify::Center);
		if(UVerticalBoxSlot* Slot=Parent->AddChildToVerticalBox(Label))Slot->SetPadding(FMargin(8.f,6.f));
		return Label;
	}
}

UButton* UPauseMenuWidget::AddButton(UVerticalBox* Parent,const FText& Label)
{
	UButton* Button=WidgetTree->ConstructWidget<UButton>();
	Button->SetBackgroundColor(FLinearColor(.07f,.22f,.34f,1.f));
	UTextBlock* Text=WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo Font=Text->Font;
	Font.Size=22;
	Text->SetFont(Font);
	Button->AddChild(Text);
	if(UButtonSlot* ButtonSlot=Cast<UButtonSlot>(Text->Slot))ButtonSlot->SetPadding(FMargin(20.f,12.f));
	if(UVerticalBoxSlot* BoxSlot=Parent->AddChildToVerticalBox(Button))BoxSlot->SetPadding(FMargin(8.f,5.f));
	return Button;
}

void UPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	bIsFocusable=true;

	UOverlay* Root=WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget=Root;
	UBorder* DimBackground=WidgetTree->ConstructWidget<UBorder>();
	DimBackground->SetBrushColor(FLinearColor(0.f,0.f,0.f,.72f));
	if(UOverlaySlot* BackgroundOverlaySlot=Root->AddChildToOverlay(DimBackground))
	{
		BackgroundOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		BackgroundOverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* MenuWidth=WidgetTree->ConstructWidget<USizeBox>();
	MenuWidth->SetWidthOverride(470.f);
	if(UOverlaySlot* MenuOverlaySlot=Root->AddChildToOverlay(MenuWidth))
	{
		MenuOverlaySlot->SetHorizontalAlignment(HAlign_Center);
		MenuOverlaySlot->SetVerticalAlignment(VAlign_Center);
	}
	UBorder* Panel=WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(.018f,.052f,.08f,.98f));
	Panel->SetPadding(FMargin(30.f,26.f));
	MenuWidth->AddChild(Panel);
	UVerticalBox* Menu=WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->AddChild(Menu);

	AddPauseText(WidgetTree,Menu,FText::FromString(TEXT("\u041f\u0410\u0423\u0417\u0410")),42,FLinearColor(.78f,.9f,1.f));
	ModeText=AddPauseText(WidgetTree,Menu,FText::GetEmpty(),17,FLinearColor(.58f,.76f,.88f));
	UButton* ContinueButton=AddButton(Menu,FText::FromString(TEXT("\u041f\u0420\u041e\u0414\u041e\u041b\u0416\u0418\u0422\u042c")));
	UButton* MainMenuButton=AddButton(Menu,FText::FromString(TEXT("\u0412 \u0413\u041b\u0410\u0412\u041d\u041e\u0415 \u041c\u0415\u041d\u042e")));
	UButton* ExitButton=AddButton(Menu,FText::FromString(TEXT("\u0412\u042b\u0419\u0422\u0418 \u0412 WINDOWS")));
	ContinueButton->OnClicked.AddDynamic(this,&UPauseMenuWidget::ContinueClicked);
	MainMenuButton->OnClicked.AddDynamic(this,&UPauseMenuWidget::MainMenuClicked);
	ExitButton->OnClicked.AddDynamic(this,&UPauseMenuWidget::ExitWindowsClicked);
	SetSinglePlayerPaused(bSinglePlayerPaused);
}

void UPauseMenuWidget::SetSinglePlayerPaused(bool bInSinglePlayerPaused)
{
	bSinglePlayerPaused=bInSinglePlayerPaused;
	if(!ModeText)return;
	ModeText->SetText(FText::FromString(bSinglePlayerPaused
		?TEXT("\u041e\u0414\u0418\u041d\u041e\u0427\u041d\u0410\u042f \u0418\u0413\u0420\u0410 \u041f\u0420\u0418\u041e\u0421\u0422\u0410\u041d\u041e\u0412\u041b\u0415\u041d\u0410")
		:TEXT("\u0421\u0415\u0422\u0415\u0412\u0410\u042f \u0418\u0413\u0420\u0410 \u041f\u0420\u041e\u0414\u041e\u041b\u0416\u0410\u0415\u0422\u0421\u042f")));
}

FReply UPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry,const FKeyEvent& InKeyEvent)
{
	if(InKeyEvent.GetKey()==EKeys::Escape)
	{
		ContinueClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry,InKeyEvent);
}

void UPauseMenuWidget::ContinueClicked()
{
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->ClosePauseMenu();
}

void UPauseMenuWidget::MainMenuClicked()
{
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->ReturnToMainMenu();
}

void UPauseMenuWidget::ExitWindowsClicked()
{
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->QuitToDesktop();
}
