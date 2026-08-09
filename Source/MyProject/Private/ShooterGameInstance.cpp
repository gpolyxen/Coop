#include "ShooterGameInstance.h"

#include "HealthArmorComponent.h"
#include "ShooterCharacter.h"
#include "ShooterSaveGame.h"
#include "WeaponBase.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "UObject/SoftObjectPath.h"

const FString UShooterGameInstance::SaveSlot=TEXT("ShooterBedSave");
const FString UShooterGameInstance::GameMap=TEXT("/Game/OpenWorld/OpenWorld");
const FString UShooterGameInstance::MenuMap=TEXT("/Game/MainMenu/MainMenu");

UShooterGameInstance::UShooterGameInstance(){}

void UShooterGameInstance::Init()
{
	Super::Init();
	if(IOnlineSubsystem* Online=IOnlineSubsystem::Get())SessionInterface=Online->GetSessionInterface();
	SetStatus(SessionInterface.IsValid()?TEXT("Готово"):TEXT("Сетевая подсистема недоступна"));
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

bool UShooterGameInstance::HasSaveGame()const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlot,0);
}

void UShooterGameInstance::StartNewGame()
{
	PendingSave=nullptr;
	if(HasSaveGame())UGameplayStatics::DeleteGameInSlot(SaveSlot,0);
	UGameplayStatics::OpenLevel(this,FName(*GameMap));
}

void UShooterGameInstance::ContinueGame()
{
	PendingSave=Cast<UShooterSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot,0));
	if(!PendingSave){SetStatus(TEXT("Сохранение не найдено"));return;}
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
	const bool bSaved=UGameplayStatics::SaveGameToSlot(Save,SaveSlot,0);
	if(bSaved)PendingSave=Save;
	UE_LOG(LogTemp,Display,TEXT("Bed save %s at %s"),bSaved?TEXT("succeeded"):TEXT("failed"),*Save->PlayerTransform.GetLocation().ToString());
	return bSaved;
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
	if(Character->Health)Character->Health->Health=FMath::Clamp(PendingSave->Health,1.f,Character->Health->MaxHealth);
	Character->CharacterLevel=FMath::Max(1,PendingSave->CharacterLevel);
	Character->Experience=FMath::Max(0,PendingSave->Experience);
	Character->TotalExperience=FMath::Max(0,PendingSave->TotalExperience);
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
	SetStatus(TEXT("LAN-сессия создана"));
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

void UShooterGameInstance::OnJoinSessionComplete(FName,EOnJoinSessionCompleteResult::Type Result)
{
	if(SessionInterface.IsValid())SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
	if(Result!=EOnJoinSessionCompleteResult::Success){SetStatus(TEXT("Не удалось подключиться"));return;}
	FString Address;
	if(!SessionInterface->GetResolvedConnectString(NAME_GameSession,Address)){SetStatus(TEXT("Адрес сервера не получен"));return;}
	if(APlayerController* PC=GetFirstLocalPlayerController())PC->ClientTravel(Address,TRAVEL_Absolute);
}

void UShooterGameInstance::ReturnToMainMenu()
{
	if(SessionInterface.IsValid()&&SessionInterface->GetNamedSession(NAME_GameSession))SessionInterface->DestroySession(NAME_GameSession);
	UGameplayStatics::OpenLevel(this,FName(*MenuMap));
}

void UShooterGameInstance::ClearSessionDelegates()
{
	if(!SessionInterface.IsValid())return;
	if(CreateSessionHandle.IsValid())SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
	if(DestroySessionHandle.IsValid())SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
	if(FindSessionsHandle.IsValid())SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
	if(JoinSessionHandle.IsValid())SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
}
