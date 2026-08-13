#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryChanged);

UCLASS(ClassGroup=(Shooter), meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UInventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintCallable) bool AddItem(FName ItemId, int32 Quantity);
	UFUNCTION(BlueprintCallable) bool RemoveItem(FName ItemId, int32 Quantity);
	UFUNCTION(BlueprintCallable) bool UpgradeCapacity(int32 NewMaxSlots);
	UFUNCTION(BlueprintPure) int32 GetQuantity(FName ItemId) const;
	UFUNCTION(BlueprintPure) bool CanAddItems(FName FirstItemId,int32 FirstQuantity,FName SecondItemId,int32 SecondQuantity) const;
	UFUNCTION(BlueprintPure) bool HasItems(FName FirstItemId,int32 FirstQuantity,FName SecondItemId,int32 SecondQuantity) const;
	UFUNCTION(BlueprintCallable) bool ConsumeItems(FName FirstItemId,int32 FirstQuantity,FName SecondItemId,int32 SecondQuantity);
	const FItemDefinition* FindDefinition(FName ItemId) const;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) UDataTable* ItemDefinitions = nullptr;
	UPROPERTY(ReplicatedUsing=OnRep_Capacity, EditDefaultsOnly, BlueprintReadOnly) int32 MaxSlots = 6;
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly) float MaxWeight = 500.f;
	UPROPERTY(ReplicatedUsing=OnRep_Items, VisibleAnywhere, BlueprintReadOnly) TArray<FInventoryEntry> Items;
	UPROPERTY(BlueprintAssignable) FInventoryChanged OnInventoryChanged;
private:
	UFUNCTION() void OnRep_Items();
	UFUNCTION() void OnRep_Capacity();
	float CurrentWeight() const;
};
