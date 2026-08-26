#include "ShooterGameInstance.h"

#include "HealthArmorComponent.h"
#include "InventoryComponent.h"
#include "PauseMenuWidget.h"
#include "ShooterCharacter.h"
#include "ShooterSaveGame.h"
#include "WeaponBase.h"
#include "BuildableStructure.h"
#include "StorageChest.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "SocketSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "EngineUtils.h"

const FString UShooterGameInstance::SaveSlot=TEXT("ShooterBedSave");
const FString UShooterGameInstance::GameMap=TEXT("/Game/OpenWorld/OpenWorld");
const FString UShooterGameInstance::MenuMap=TEXT("/Game/MainMenu/MainMenu");

UShooterGameInstance::UShooterGameInstance(){}

void UShooterGameInstance::Init()
{
	Super::Init();
	if(IOnlineSubsystem* Online=IOnlineSubsystem::Get())SessionInterface=Online->GetSessionInterface();
	SetStatus(SessionInterface.IsValid()?GetLocalLanAddressText():TEXT("Сетевая подсистема недоступна"));
}

FString UShooterGameInstance::GetLocalLanAddressText()const
{
	TArray<FString> Addresses;
	if(ISocketSubsystem* SocketSubsystem=ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	{
		TArray<TSharedPtr<FInternetAddr>> AdapterAddresses;
		if(SocketSubsystem->GetLocalAdapterAddresses(AdapterAddresses))
		{
			for(const TSharedPtr<FInternetAddr>& AdapterAddress:AdapterAddresses)
			{
				if(!AdapterAddress.IsValid()||!AdapterAddress->IsValid())continue;
				const FString Address=AdapterAddress->ToString(false);
				if(Address.IsEmpty()||Address==TEXT("0.0.0.0")||Address.StartsWith(TEXT("127."))||Address.Contains(TEXT(":")))continue;
				Addresses.AddUnique(Address+TEXT(":7777"));
			}
		}
	}
	return Addresses.Num()>0
		?FString::Printf(TEXT("IP этого ПК: %s"),*FString::Join(Addresses,TEXT("   ")))
		:TEXT("IP этого ПК не определён — посмотрите IPv4 через ipconfig");
}

void UShooterGameInstance::Shutdown()
{
	ClearSessionDelegates();
	Super::Shutdown();
}

void UShooterGameInstance::SetStatus(const FString& NewStatus)
{
	MenuStatus=NewStatus;
	OnMenuStateChanged.Broadcast();
	UE_LOG(LogTemp,Display,TEXT("Menu: %s"),*NewStatus);
}

void UShooterGameInstance::PrepareForGameplayTravel()
{
	ClosePauseMenu();
	UWidgetLayoutLibrary::RemoveAllWidgets(this);
	if(APlayerController* PC=GetFirstLocalPlayerController())
	{
		PC->bShowMouseCursor=false;
		PC->bEnableClickEvents=false;
		PC->bEnableMouseOverEvents=false;
		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();
		PC->SetPause(false);
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

bool UShooterGameInstance::IsPauseMenuOpen()const
{
	return PauseMenuWidget&&PauseMenuWidget->IsInViewport();
}

void UShooterGameInstance::TogglePauseMenu()
{
	if(IsPauseMenuOpen())
	{
		ClosePauseMenu();
		return;
	}
	UWorld* World=GetWorld();
	APlayerController* PC=GetFirstLocalPlayerController();
	if(!World||!PC||!PC->IsLocalController())return;

	if(AShooterCharacter* Character=Cast<AShooterCharacter>(PC->GetPawn()))Character->StopGameplayActionsForMenu();
	PauseMenuWidget=CreateWidget<UPauseMenuWidget>(PC,UPauseMenuWidget::StaticClass());
	if(!PauseMenuWidget)return;
	const bool bPauseSinglePlayer=World->GetNetMode()==NM_Standalone;
	PauseMenuWidget->SetSinglePlayerPaused(bPauseSinglePlayer);
	PauseMenuWidget->AddToViewport(500);

	PC->bShowMouseCursor=true;
	PC->bEnableClickEvents=true;
	PC->bEnableMouseOverEvents=true;
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);
	PauseMenuWidget->SetKeyboardFocus();
	if(bPauseSinglePlayer)PC->SetPause(true);
	UE_LOG(LogTemp,Display,TEXT("Pause menu opened: net mode %d, world paused %s"),static_cast<int32>(World->GetNetMode()),bPauseSinglePlayer?TEXT("true"):TEXT("false"));
}

void UShooterGameInstance::ClosePauseMenu()
{
	if(PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget=nullptr;
	}
	if(APlayerController* PC=GetFirstLocalPlayerController())
	{
		if(GetWorld()&&GetWorld()->GetNetMode()==NM_Standalone&&UGameplayStatics::IsGamePaused(this))PC->SetPause(false);
		PC->bShowMouseCursor=false;
		PC->bEnableClickEvents=false;
		PC->bEnableMouseOverEvents=false;
		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

bool UShooterGameInstance::HasSaveGame()const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlot,0);
}

void UShooterGameInstance::StartNewGame()
{
	PendingSave=nullptr;
	bPendingWorldRestored=false;
	if(HasSaveGame())UGameplayStatics::DeleteGameInSlot(SaveSlot,0);
	PrepareForGameplayTravel();
	UGameplayStatics::OpenLevel(this,FName(*GameMap));
}

void UShooterGameInstance::ContinueGame()
{
	PendingSave=Cast<UShooterSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot,0));
	if(!PendingSave){SetStatus(TEXT("Сохранение не найдено"));return;}
	bPendingWorldRestored=false;
	PrepareForGameplayTravel();
	UGameplayStatics::OpenLevel(this,FName(*PendingSave->MapPath));
}

bool UShooterGameInstance::SavePlayerAtBed(AShooterCharacter* Character)
{
	if(!Character||Character->IsDead())return false;
	UShooterSaveGame* Save=Cast<UShooterSaveGame>(UGameplayStatics::CreateSaveGameObject(UShooterSaveGame::StaticClass()));
	if(!Save)return false;
	Save->PlayerTransform=Character->GetActorTransform();
	Save->Health=Character->Health?Character->Health->Health:100.f;
	Save->CharacterLevel=Character->CharacterLevel;
	Save->Experience=Character->Experience;
	Save->TotalExperience=Character->TotalExperience;
	Save->SkillPoints=Character->SkillPoints;
	Save->UnlockedSkills=Character->UnlockedSkills;
	Save->bLastLifeConsumed=Character->bLastLifeConsumed;
	if(Character->Inventory)
	{
		Save->InventoryMaxSlots=Character->Inventory->MaxSlots;
		Save->InventoryMaxWeight=Character->Inventory->MaxWeight;
		Save->InventoryItems=Character->Inventory->Items;
	}
	Save->ActiveWeaponSlot=Character->ActiveWeaponSlot;
	Save->SavedAt=FDateTime::Now();
	for(AWeaponBase* Weapon:Character->WeaponSlots)
	{
		if(!Weapon)continue;
		FSavedWeaponData Data;
		Data.WeaponClassPath=Weapon->GetClass()->GetPathName();
		Data.AmmoInMagazine=Weapon->AmmoInMagazine;
		Data.ReserveAmmo=Weapon->ReserveAmmo;
		Save->Weapons.Add(Data);
	}
	if(UWorld* World=Character->GetWorld())
	{
		for(TActorIterator<ABuildableStructure> It(World);It;++It)
		{
			ABuildableStructure* Structure=*It;
			if(!Structure||Structure->IsConstructionPreview()||Structure->IsCollapsing())continue;
			FSavedBuildableData Data;
			Data.StructureClassPath=Structure->GetClass()->GetPathName();
			Data.Transform=Structure->GetActorTransform();
			Data.Health=Structure->StructureHealth;
			if(const AWoodGate* Gate=Cast<AWoodGate>(Structure))Data.bGateOpen=Gate->bOpen;
			if(const AStorageChest* Chest=Cast<AStorageChest>(Structure))
			{
				if(Chest->Storage)Data.StoredItems=Chest->Storage->Items;
				for(const AWeaponBase* Weapon:Chest->StoredWeapons)
				{
					if(!Weapon)continue;
					FSavedWeaponData WeaponData;
					WeaponData.WeaponClassPath=Weapon->GetClass()->GetPathName();
					WeaponData.AmmoInMagazine=Weapon->AmmoInMagazine;
					WeaponData.ReserveAmmo=Weapon->ReserveAmmo;
					Data.StoredWeapons.Add(WeaponData);
				}
			}
			Save->BuildableStructures.Add(Data);
		}
	}
	const bool bSaved=UGameplayStatics::SaveGameToSlot(Save,SaveSlot,0);
	if(bSaved)PendingSave=Save;
	UE_LOG(LogTemp,Display,TEXT("Bed save %s at %s"),bSaved?TEXT("succeeded"):TEXT("failed"),*Save->PlayerTransform.GetLocation().ToString());
	return bSaved;
}

bool UShooterGameInstance::RestorePendingWorld(UWorld* World)
{
	if(!PendingSave||!World||bPendingWorldRestored||World->GetNetMode()==NM_Client)return false;
	bPendingWorldRestored=true;
	if(PendingSave->BuildableStructures.Num()==0)return false;
	for(TActorIterator<ABuildableStructure> It(World);It;++It)if(ABuildableStructure* Existing=*It)Existing->Destroy();
	int32 RestoredCount=0;
	for(const FSavedBuildableData& Data:PendingSave->BuildableStructures)
	{
		UClass* StructureClass=FSoftClassPath(Data.StructureClassPath).TryLoadClass<ABuildableStructure>();
		if(!StructureClass)continue;
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ABuildableStructure* Structure=World->SpawnActor<ABuildableStructure>(StructureClass,Data.Transform,Parameters);
		if(!Structure)continue;
		Structure->StructureHealth=FMath::Clamp(Data.Health,1.f,Structure->MaxStructureHealth);
		if(AWoodGate* Gate=Cast<AWoodGate>(Structure))Gate->SetOpenForLoad(Data.bGateOpen);
		if(AStorageChest* Chest=Cast<AStorageChest>(Structure))
		{
			if(Chest->Storage)
			{
				Chest->Storage->Items=Data.StoredItems;
				Chest->Storage->MaxSlots=FMath::Max(0,20-Data.StoredWeapons.Num());
				Chest->Storage->OnInventoryChanged.Broadcast();
			}
			for(const FSavedWeaponData& WeaponData:Data.StoredWeapons)
			{
				UClass* WeaponClass=FSoftClassPath(WeaponData.WeaponClassPath).TryLoadClass<AWeaponBase>();
				if(!WeaponClass)continue;
				FActorSpawnParameters WeaponParameters;WeaponParameters.Owner=Chest;WeaponParameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if(AWeaponBase* Weapon=World->SpawnActor<AWeaponBase>(WeaponClass,FTransform::Identity,WeaponParameters))
				{
					Weapon->AmmoInMagazine=FMath::Clamp(WeaponData.AmmoInMagazine,0,Weapon->Stats.MagazineSize);
					Weapon->ReserveAmmo=FMath::Clamp(WeaponData.ReserveAmmo,0,Weapon->MaxReserveAmmo);
					Weapon->AttachToActor(Chest,FAttachmentTransformRules::KeepRelativeTransform);
					Weapon->SetActorHiddenInGame(true);
					Chest->StoredWeapons.Add(Weapon);
				}
			}
		}
		++RestoredCount;
	}
	UE_LOG(LogTemp,Display,TEXT("Restored %d/%d saved buildable structures"),RestoredCount,PendingSave->BuildableStructures.Num());
	return RestoredCount>0;
}

bool UShooterGameInstance::GetPendingPlayerTransform(FTransform& OutTransform)const
{
	if(!PendingSave)return false;
	OutTransform=PendingSave->PlayerTransform;
	return true;
}

bool UShooterGameInstance::ApplyPendingSave(AShooterCharacter* Character)
{
	if(!PendingSave||!Character||!Character->HasAuthority())return false;
	Character->SetActorTransform(PendingSave->PlayerTransform,false,nullptr,ETeleportType::TeleportPhysics);
	Character->CharacterLevel=FMath::Max(1,PendingSave->CharacterLevel);
	Character->Experience=FMath::Max(0,PendingSave->Experience);
	Character->TotalExperience=FMath::Max(0,PendingSave->TotalExperience);
	Character->SkillPoints=FMath::Max(0,PendingSave->SkillPoints);
	Character->UnlockedSkills=PendingSave->UnlockedSkills;
	Character->bLastLifeConsumed=PendingSave->bLastLifeConsumed;
	Character->LastLifeInvulnerableUntil=0.f;
	Character->ApplyUnlockedSkillEffects();
	if(Character->Health)Character->Health->Health=FMath::Clamp(PendingSave->Health,1.f,Character->Health->MaxHealth);
	if(Character->Inventory)
	{
		Character->Inventory->MaxSlots=FMath::Clamp(PendingSave->InventoryMaxSlots,6,20);
		Character->Inventory->MaxWeight=FMath::Max3(600.f,Character->Inventory->MaxSlots*100.f,PendingSave->InventoryMaxWeight);
		Character->Inventory->OverrideMaxStack=100;
		Character->Inventory->bAllowMultipleStacks=true;
		Character->Inventory->Items=PendingSave->InventoryItems;
		Character->Inventory->OnInventoryChanged.Broadcast();
	}
	for(const FSavedWeaponData& Data:PendingSave->Weapons)
	{
		UClass* WeaponClass=FSoftClassPath(Data.WeaponClassPath).TryLoadClass<AWeaponBase>();
		if(!WeaponClass)continue;
		Character->EquipWeapon(WeaponClass);
		if(Character->EquippedWeapon)
		{
			Character->EquippedWeapon->AmmoInMagazine=FMath::Clamp(Data.AmmoInMagazine,0,Character->EquippedWeapon->Stats.MagazineSize);
			Character->EquippedWeapon->ReserveAmmo=FMath::Clamp(Data.ReserveAmmo,0,Character->EquippedWeapon->MaxReserveAmmo);
		}
	}
	Character->SetActiveWeaponSlotForLoad(PendingSave->ActiveWeaponSlot);
	UE_LOG(LogTemp,Display,TEXT("Loaded bed save: health=%.0f level=%d xp=%d weapons=%d location=%s"),
		Character->Health?Character->Health->Health:0.f,Character->CharacterLevel,Character->Experience,
		Character->WeaponSlots.Num(),*Character->GetActorLocation().ToString());
	PendingSave=nullptr;
	return true;
}

void UShooterGameInstance::HostLanGame()
{
	PendingSave=nullptr;
	if(!SessionInterface.IsValid()){SetStatus(TEXT("LAN недоступна"));return;}
	if(SessionInterface->GetNamedSession(NAME_GameSession))
	{
		bCreateSessionAfterDestroy=true;
		DestroySessionHandle=SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this,&UShooterGameInstance::OnDestroySessionComplete));
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}
	CreateLanSession();
}

void UShooterGameInstance::CreateLanSession()
{
	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch=true;
	Settings.NumPublicConnections=4;
	Settings.NumPrivateConnections=0;
	Settings.bShouldAdvertise=true;
	Settings.bAllowJoinInProgress=true;
	Settings.bAllowJoinViaPresence=false;
	Settings.bUsesPresence=false;
	Settings.Set(FName(TEXT("SERVER_NAME")),FString(TEXT("Локальная игра")),EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(SETTING_MAPNAME,GameMap,EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	CreateSessionHandle=SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this,&UShooterGameInstance::OnCreateSessionComplete));
	SetStatus(TEXT("Создание LAN-сессии..."));
	if(!SessionInterface->CreateSession(0,NAME_GameSession,Settings))OnCreateSessionComplete(NAME_GameSession,false);
}

void UShooterGameInstance::OnCreateSessionComplete(FName,bool bSuccess)
{
	if(SessionInterface.IsValid())SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
	if(!bSuccess){SetStatus(TEXT("Не удалось создать LAN-сессию"));return;}
	SetStatus(FString::Printf(TEXT("LAN-сессия создана. %s"),*GetLocalLanAddressText()));
	PrepareForGameplayTravel();
	if(UWorld* World=GetWorld())World->ServerTravel(GameMap+TEXT("?listen"));
}

void UShooterGameInstance::OnDestroySessionComplete(FName,bool)
{
	if(SessionInterface.IsValid())SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
	if(bCreateSessionAfterDestroy){bCreateSessionAfterDestroy=false;CreateLanSession();}
}

void UShooterGameInstance::FindLanGames()
{
	FoundLanGames.Reset();
	FoundLanResultIndices.Reset();
	if(!SessionInterface.IsValid()){SetStatus(TEXT("LAN недоступна"));return;}
	SessionSearch=MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery=true;
	SessionSearch->MaxSearchResults=50;
	SessionSearch->PingBucketSize=50;
	FindSessionsHandle=SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this,&UShooterGameInstance::OnFindSessionsComplete));
	SetStatus(TEXT("Поиск игр в локальной сети..."));
	if(!SessionInterface->FindSessions(0,SessionSearch.ToSharedRef()))OnFindSessionsComplete(false);
}

void UShooterGameInstance::OnFindSessionsComplete(bool bSuccess)
{
	if(SessionInterface.IsValid())SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
	FoundLanGames.Reset();
	FoundLanResultIndices.Reset();
	if(bSuccess&&SessionSearch.IsValid())
	{
		TSet<FString> SeenSessionIds;
		for(int32 SearchIndex=0;SearchIndex<SessionSearch->SearchResults.Num();++SearchIndex)
		{
			const FOnlineSessionSearchResult& Result=SessionSearch->SearchResults[SearchIndex];
			const FString SessionId=Result.GetSessionIdStr();
			if(!SessionId.IsEmpty()&&SeenSessionIds.Contains(SessionId))continue;
			if(!SessionId.IsEmpty())SeenSessionIds.Add(SessionId);
			FString ServerName;
			Result.Session.SessionSettings.Get(FName(TEXT("SERVER_NAME")),ServerName);
			if(ServerName.IsEmpty())ServerName=Result.Session.OwningUserName;
			const int32 CurrentPlayers=Result.Session.SessionSettings.NumPublicConnections-Result.Session.NumOpenPublicConnections;
			FoundLanGames.Add(FString::Printf(TEXT("%s   %d/%d   Ping %d"),*ServerName,CurrentPlayers,Result.Session.SessionSettings.NumPublicConnections,Result.PingInMs));
			FoundLanResultIndices.Add(SearchIndex);
		}
	}
	SetStatus(FoundLanGames.Num()>0?FString::Printf(TEXT("Найдено игр: %d"),FoundLanGames.Num()):TEXT("LAN-игры не найдены"));
}

void UShooterGameInstance::JoinLanGame(int32 ResultIndex)
{
	if(!SessionInterface.IsValid()||!SessionSearch.IsValid()||!FoundLanResultIndices.IsValidIndex(ResultIndex)){SetStatus(TEXT("Сессия больше недоступна"));return;}
	const int32 SearchResultIndex=FoundLanResultIndices[ResultIndex];
	if(!SessionSearch->SearchResults.IsValidIndex(SearchResultIndex)){SetStatus(TEXT("Сессия больше недоступна"));return;}
	JoinSessionHandle=SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this,&UShooterGameInstance::OnJoinSessionComplete));
	SetStatus(TEXT("Подключение..."));
	if(!SessionInterface->JoinSession(0,NAME_GameSession,SessionSearch->SearchResults[SearchResultIndex]))OnJoinSessionComplete(NAME_GameSession,EOnJoinSessionCompleteResult::UnknownError);
}

void UShooterGameInstance::JoinLanByAddress(const FString& Address)
{
	FString ConnectAddress=Address;
	ConnectAddress.TrimStartAndEndInline();
	ConnectAddress.RemoveFromStart(TEXT("open "),ESearchCase::IgnoreCase);
	ConnectAddress.RemoveFromStart(TEXT("udp://"),ESearchCase::IgnoreCase);
	ConnectAddress.RemoveFromStart(TEXT("unreal://"),ESearchCase::IgnoreCase);
	ConnectAddress.RemoveFromEnd(TEXT("/"));
	if(ConnectAddress.IsEmpty())
	{
		SetStatus(TEXT("Введите IPv4 первого ПК, например 192.168.1.25"));
		return;
	}
	if(!ConnectAddress.Contains(TEXT(":")))ConnectAddress+=TEXT(":7777");

	FString HostPart;
	FString PortPart;
	if(!ConnectAddress.Split(TEXT(":"),&HostPart,&PortPart,ESearchCase::IgnoreCase,ESearchDir::FromEnd))
	{
		SetStatus(TEXT("Неверный адрес. Пример: 192.168.1.25:7777"));
		return;
	}
	FIPv4Address ParsedAddress;
	if(!FIPv4Address::Parse(HostPart,ParsedAddress)||FCString::Atoi(*PortPart)<=0||FCString::Atoi(*PortPart)>65535)
	{
		SetStatus(TEXT("Неверный адрес. Пример: 192.168.1.25:7777"));
		return;
	}

	SetStatus(FString::Printf(TEXT("Прямое подключение к %s..."),*ConnectAddress));
	UE_LOG(LogTemp,Display,TEXT("LAN direct connect to %s"),*ConnectAddress);
	PrepareForGameplayTravel();
	if(APlayerController* PC=GetFirstLocalPlayerController())PC->ClientTravel(ConnectAddress,TRAVEL_Absolute);
}

void UShooterGameInstance::OnJoinSessionComplete(FName,EOnJoinSessionCompleteResult::Type Result)
{
	if(SessionInterface.IsValid())SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
	if(Result!=EOnJoinSessionCompleteResult::Success){SetStatus(TEXT("Не удалось подключиться"));return;}
	FString Address;
	if(!SessionInterface->GetResolvedConnectString(NAME_GameSession,Address)){SetStatus(TEXT("Адрес сервера не получен"));return;}
	PrepareForGameplayTravel();
	if(APlayerController* PC=GetFirstLocalPlayerController())PC->ClientTravel(Address,TRAVEL_Absolute);
}

void UShooterGameInstance::ReturnToMainMenu()
{
	ClosePauseMenu();
	UWidgetLayoutLibrary::RemoveAllWidgets(this);
	bCreateSessionAfterDestroy=false;
	if(SessionInterface.IsValid()&&SessionInterface->GetNamedSession(NAME_GameSession))SessionInterface->DestroySession(NAME_GameSession);
	UGameplayStatics::OpenLevel(this,FName(*MenuMap));
}

void UShooterGameInstance::QuitToDesktop()
{
	APlayerController* PC=GetFirstLocalPlayerController();
	ClosePauseMenu();
	bCreateSessionAfterDestroy=false;
	if(SessionInterface.IsValid()&&SessionInterface->GetNamedSession(NAME_GameSession))SessionInterface->DestroySession(NAME_GameSession);
	UKismetSystemLibrary::QuitGame(this,PC,EQuitPreference::Quit,false);
}

void UShooterGameInstance::ClearSessionDelegates()
{
	if(!SessionInterface.IsValid())return;
	if(CreateSessionHandle.IsValid())SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
	if(DestroySessionHandle.IsValid())SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
	if(FindSessionsHandle.IsValid())SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
	if(JoinSessionHandle.IsValid())SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
}
