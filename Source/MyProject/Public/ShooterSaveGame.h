#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ShooterTypes.h"
#include "ShooterSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FSavedWeaponData
{
	GENERATED_BODY()
	UPROPERTY()FString WeaponClassPath;
	UPROPERTY()int32 AmmoInMagazine=0;
	UPROPERTY()int32 ReserveAmmo=0;
};

USTRUCT()
struct FSavedBuildableData
{
	GENERATED_BODY()
	UPROPERTY()FString StructureClassPath;
	UPROPERTY()FTransform Transform;
	UPROPERTY()float Health=0.f;
	UPROPERTY()bool bGateOpen=false;
	UPROPERTY()TArray<FInventoryEntry> StoredItems;
	UPROPERTY()TArray<FSavedWeaponData> StoredWeapons;
};

UCLASS()
class MYPROJECT_API UShooterSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY()FString MapPath=TEXT("/Game/OpenWorld/OpenWorld");
	UPROPERTY()FTransform PlayerTransform;
	UPROPERTY()float Health=100.f;
	UPROPERTY()int32 CharacterLevel=1;
	UPROPERTY()int32 Experience=0;
	UPROPERTY()int32 TotalExperience=0;
	UPROPERTY()int32 SkillPoints=0;
	UPROPERTY()TArray<EShooterSkill> UnlockedSkills;
	UPROPERTY()bool bLastLifeConsumed=false;
	UPROPERTY()int32 InventoryMaxSlots=6;
	UPROPERTY()float InventoryMaxWeight=45.f;
	UPROPERTY()TArray<FInventoryEntry> InventoryItems;
	UPROPERTY()TArray<FSavedWeaponData> Weapons;
	UPROPERTY()int32 ActiveWeaponSlot=INDEX_NONE;
	UPROPERTY()TArray<FSavedBuildableData> BuildableStructures;
	UPROPERTY()FDateTime SavedAt;
};
