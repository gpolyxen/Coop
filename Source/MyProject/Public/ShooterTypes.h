#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ShooterTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8 { Weapon, Ammunition, Armor, Consumable, Quest };

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8 { None, Primary, Secondary, Helmet, Vest };

USTRUCT(BlueprintType)
struct MYPROJECT_API FItemDefinition : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ItemId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EItemType Type = EItemType::Consumable;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EEquipmentSlot Slot = EEquipmentSlot::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.01")) float Weight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 MaxStack = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0", ClampMax="1.0")) float ArmorDamageReduction = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<class AWeaponBase> WeaponClass;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FInventoryEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ItemId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Quantity = 0;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FWeaponStats
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1.0")) float Damage = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1.0")) float MuzzleVelocity = 90000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.01")) float ProjectileMassGrams = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0")) float DragCoefficient = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0")) float WindInfluence = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0")) float GravityScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1.0")) float ProjectileLifeSeconds = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0")) float RoundsPerMinute = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 MagazineSize = 30;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0")) float ReloadSeconds = 2.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0")) float SpreadDegrees = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0")) float NoiseLoudness = 1.f;
};
