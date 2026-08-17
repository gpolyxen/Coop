#pragma once
#include "CoreMinimal.h"
#include "BuildableStructure.h"
#include "StorageChest.generated.h"
class UInventoryComponent;class UStaticMeshComponent;class UTextRenderComponent;class AWeaponBase;
UCLASS()
class MYPROJECT_API AStorageChest : public ABuildableStructure
{
	GENERATED_BODY()
public:AStorageChest();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	FName GetStoredWeaponId(const AWeaponBase* Weapon)const;
	AWeaponBase* FindStoredWeapon(FName ItemId)const;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Box=nullptr;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Lid=nullptr;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UTextRenderComponent* UseText=nullptr;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UInventoryComponent* Storage=nullptr;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly)TArray<AWeaponBase*> StoredWeapons;
};
