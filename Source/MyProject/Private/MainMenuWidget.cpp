#include "MainMenuWidget.h"

#include "ShooterGameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

static UTextBlock* AddMenuText(UWidgetTree* Tree,UVerticalBox* Parent,const FString& Text,int32 Size,const FLinearColor& Color)
{
	UTextBlock* Label=Tree->ConstructWidget<UTextBlock>();
	Label->SetText(FText::FromString(Text));
	Label->SetColorAndOpacity(FSlateColor(Color));
	FSlateFontInfo Font=Label->Font;Font.Size=Size;Label->SetFont(Font);
	Label->SetJustification(ETextJustify::Center);
	if(UVerticalBoxSlot* BoxSlot=Parent->AddChildToVerticalBox(Label))BoxSlot->SetPadding(FMargin(8.f,5.f));
	return Label;
}

void ULanSessionEntryWidget::Setup(int32 InIndex,const FString& InLabel){ResultIndex=InIndex;Label=InLabel;if(LabelText)LabelText->SetText(FText::FromString(Label));}

void ULanSessionEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UButton* Button=WidgetTree->ConstructWidget<UButton>();
	WidgetTree->RootWidget=Button;
	Button->SetBackgroundColor(FLinearColor(.12f,.32f,.5f,1.f));
	LabelText=WidgetTree->ConstructWidget<UTextBlock>();
	LabelText->SetText(FText::FromString(Label));
	LabelText->SetJustification(ETextJustify::Center);
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo Font=LabelText->Font;Font.Size=18;LabelText->SetFont(Font);
	Button->AddChild(LabelText);
	if(UButtonSlot* ButtonSlot=Cast<UButtonSlot>(LabelText->Slot))ButtonSlot->SetPadding(FMargin(14.f,8.f));
	Button->OnClicked.AddDynamic(this,&ULanSessionEntryWidget::JoinClicked);
}

void ULanSessionEntryWidget::JoinClicked()
{
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->JoinLanGame(ResultIndex);
}

UButton* UMainMenuWidget::AddButton(UVerticalBox* Parent,const FString& Label)
{
	UButton* Button=WidgetTree->ConstructWidget<UButton>();
	Button->SetBackgroundColor(FLinearColor(.08f,.24f,.38f,1.f));
	UTextBlock* Text=WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetJustification(ETextJustify::Center);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo Font=Text->Font;Font.Size=21;Text->SetFont(Font);
	Button->AddChild(Text);
	if(UButtonSlot* ButtonSlot=Cast<UButtonSlot>(Text->Slot))ButtonSlot->SetPadding(FMargin(18.f,10.f));
	if(UVerticalBoxSlot* BoxSlot=Parent->AddChildToVerticalBox(Button)){BoxSlot->SetPadding(FMargin(8.f,4.f));BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));}
	return Button;
}

void UMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UOverlay* Root=WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget=Root;
	UBorder* Background=WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(.012f,.025f,.04f,.97f));
	if(UOverlaySlot* BackgroundSlot=Root->AddChildToOverlay(Background))
	{
		BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
		BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
	}
	USizeBox* MenuWidth=WidgetTree->ConstructWidget<USizeBox>();
	MenuWidth->SetWidthOverride(560.f);
	if(UOverlaySlot* MenuSlot=Root->AddChildToOverlay(MenuWidth))
	{
		MenuSlot->SetHorizontalAlignment(HAlign_Center);
		MenuSlot->SetVerticalAlignment(VAlign_Center);
	}
	UBorder* MenuPanel=WidgetTree->ConstructWidget<UBorder>();
	MenuPanel->SetBrushColor(FLinearColor(.025f,.065f,.1f,.96f));
	MenuPanel->SetPadding(FMargin(30.f,24.f));
	MenuWidth->AddChild(MenuPanel);
	UVerticalBox* Menu=WidgetTree->ConstructWidget<UVerticalBox>();
	MenuPanel->AddChild(Menu);
	AddMenuText(WidgetTree,Menu,TEXT("SURVIVAL"),42,FLinearColor(.78f,.9f,1.f));
	AddMenuText(WidgetTree,Menu,TEXT("ОДИН ИГРОК"),24,FLinearColor(.45f,.75f,1.f));
	UButton* NewButton=AddButton(Menu,TEXT("НОВАЯ ИГРА"));
	ContinueButton=AddButton(Menu,TEXT("ПРОДОЛЖИТЬ"));
	AddMenuText(WidgetTree,Menu,TEXT("МУЛЬТИПЛЕЕР"),24,FLinearColor(.45f,.75f,1.f));
	UButton* HostButton=AddButton(Menu,TEXT("СОЗДАТЬ ИГРУ В LAN"));
	UButton* FindButton=AddButton(Menu,TEXT("НАЙТИ ИГРЫ В LAN"));
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())
	{
		AddMenuText(WidgetTree,Menu,GI->GetLocalLanAddressText(),15,FLinearColor(.55f,.8f,1.f));
	}
	AddMenuText(WidgetTree,Menu,TEXT("Если игра не найдена — введите IP первого ПК"),16,FLinearColor(.7f,.85f,.95f));
	DirectIpInput=WidgetTree->ConstructWidget<UEditableTextBox>();
	DirectIpInput->SetHintText(FText::FromString(TEXT("Например: 192.168.1.25")));
	DirectIpInput->SetJustification(ETextJustify::Center);
	if(UVerticalBoxSlot* IpSlot=Menu->AddChildToVerticalBox(DirectIpInput))IpSlot->SetPadding(FMargin(8.f,4.f));
	UButton* JoinByIpButton=AddButton(Menu,TEXT("ПОДКЛЮЧИТЬСЯ ПО IP"));
	UButton* InternetButton=AddButton(Menu,TEXT("ИГРА ПО ИНТЕРНЕТУ — ПОЗЖЕ"));
	InternetButton->SetIsEnabled(false);
	SessionList=WidgetTree->ConstructWidget<UVerticalBox>();
	if(UVerticalBoxSlot* BoxSlot=Menu->AddChildToVerticalBox(SessionList))BoxSlot->SetPadding(FMargin(5.f));
	StatusText=AddMenuText(WidgetTree,Menu,TEXT(""),17,FLinearColor(.7f,.85f,.95f));
	NewButton->OnClicked.AddDynamic(this,&UMainMenuWidget::NewGameClicked);
	ContinueButton->OnClicked.AddDynamic(this,&UMainMenuWidget::ContinueClicked);
	HostButton->OnClicked.AddDynamic(this,&UMainMenuWidget::HostLanClicked);
	FindButton->OnClicked.AddDynamic(this,&UMainMenuWidget::FindLanClicked);
	JoinByIpButton->OnClicked.AddDynamic(this,&UMainMenuWidget::JoinByIpClicked);
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->OnMenuStateChanged.AddDynamic(this,&UMainMenuWidget::RefreshMenuState);
	RefreshMenuState();
}

void UMainMenuWidget::NativeDestruct()
{
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->OnMenuStateChanged.RemoveDynamic(this,&UMainMenuWidget::RefreshMenuState);
	Super::NativeDestruct();
}

void UMainMenuWidget::RebuildSessionList()
{
	if(!SessionList)return;
	SessionList->ClearChildren();
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())
	{
		for(int32 Index=0;Index<GI->FoundLanGames.Num();++Index)
		{
			ULanSessionEntryWidget* Entry=CreateWidget<ULanSessionEntryWidget>(GetOwningPlayer(),ULanSessionEntryWidget::StaticClass());
			Entry->Setup(Index,GI->FoundLanGames[Index]);
			if(UVerticalBoxSlot* BoxSlot=SessionList->AddChildToVerticalBox(Entry))BoxSlot->SetPadding(FMargin(8.f,2.f));
		}
	}
}

void UMainMenuWidget::RefreshMenuState()
{
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())
	{
		if(ContinueButton)ContinueButton->SetIsEnabled(GI->HasSaveGame());
		if(StatusText)StatusText->SetText(FText::FromString(GI->MenuStatus));
		RebuildSessionList();
	}
}

void UMainMenuWidget::NewGameClicked(){if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->StartNewGame();}
void UMainMenuWidget::ContinueClicked(){if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->ContinueGame();}
void UMainMenuWidget::HostLanClicked(){if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->HostLanGame();}
void UMainMenuWidget::FindLanClicked(){if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())GI->FindLanGames();}
void UMainMenuWidget::JoinByIpClicked()
{
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())
	{
		GI->JoinLanByAddress(DirectIpInput?DirectIpInput->GetText().ToString():FString());
	}
}
