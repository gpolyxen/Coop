#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ShooterGameInstance.generated.h"

class AShooterCharacter;
class UShooterSaveGame;
class FOnlineSessionSearch;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMenuStateChanged);

UCLASS()
class MYPROJECT_API UShooterGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	UShooterGameInstance();
	virtual void Init()override;
	virtual void Shutdown()override;

	UFUNCTION(BlueprintCallable)void StartNewGame();
	UFUNCTION(BlueprintCallable)void ContinueGame();
	UFUNCTION(BlueprintCallable)void HostLanGame();
	UFUNCTION(BlueprintCallable)void FindLanGames();
	UFUNCTION(BlueprintCallable)void JoinLanGame(int32 ResultIndex);
	UFUNCTION(BlueprintCallable)void ReturnToMainMenu();
	UFUNCTION(BlueprintPure)bool HasSaveGame()const;
	UFUNCTION(BlueprintCallable)bool SavePlayerAtBed(AShooterCharacter* Character);
	bool ApplyPendingSave(AShooterCharacter* Character);
	bool GetPendingPlayerTransform(FTransform& OutTransform)const;

	UPROPERTY(BlueprintAssignable)FMenuStateChanged OnMenuStateChanged;
	UPROPERTY(BlueprintReadOnly)TArray<FString> FoundLanGames;
	UPROPERTY(BlueprintReadOnly)FString MenuStatus;

	static const FString SaveSlot;
	static const FString GameMap;
	static const FString MenuMap;

private:
	void SetStatus(const FString& NewStatus);
	void PrepareForGameplayTravel();
	void CreateLanSession();
	void OnCreateSessionComplete(FName SessionName,bool bSuccess);
	void OnDestroySessionComplete(FName SessionName,bool bSuccess);
	void OnFindSessionsComplete(bool bSuccess);
	void OnJoinSessionComplete(FName SessionName,EOnJoinSessionCompleteResult::Type Result);
	void ClearSessionDelegates();

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FDelegateHandle CreateSessionHandle;
	FDelegateHandle DestroySessionHandle;
	FDelegateHandle FindSessionsHandle;
	FDelegateHandle JoinSessionHandle;
	TArray<int32> FoundLanResultIndices;
	UPROPERTY()UShooterSaveGame* PendingSave=nullptr;
	bool bCreateSessionAfterDestroy=false;
};
