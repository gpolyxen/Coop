#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent() { SetIsReplicatedByDefault(true); }
const FItemDefinition* UInventoryComponent::FindDefinition(FName Id) const { return ItemDefinitions ? ItemDefinitions->FindRow<FItemDefinition>(Id, TEXT("Inventory")) : nullptr; }
int32 UInventoryComponent::GetQuantity(FName Id) const { for (const FInventoryEntry& E : Items) if (E.ItemId == Id) return E.Quantity; return 0; }
float UInventoryComponent::CurrentWeight() const { float Sum=0; for(const FInventoryEntry& E:Items){const FItemDefinition* D=FindDefinition(E.ItemId);Sum+=(D?D->Weight:1.f)*E.Quantity;}return Sum; }
bool UInventoryComponent::AddItem(FName Id, int32 Quantity)
{
	if(!GetOwner()->HasAuthority()||Quantity<=0||Id.IsNone())return false;
	const FItemDefinition* Definition=FindDefinition(Id);
	const float UnitWeight=Definition?Definition->Weight:1.f;
	const int32 MaxStack=Definition?Definition->MaxStack:99;
	if(CurrentWeight()+UnitWeight*Quantity>MaxWeight)return false;
	for(FInventoryEntry& Entry:Items)
		if(Entry.ItemId==Id&&Entry.Quantity+Quantity<=MaxStack)
		{
			Entry.Quantity+=Quantity;
			OnInventoryChanged.Broadcast();
			return true;
		}
	if(Items.Num()>=MaxSlots||Quantity>MaxStack)return false;
	FInventoryEntry Entry;Entry.ItemId=Id;Entry.Quantity=Quantity;Items.Add(Entry);
	OnInventoryChanged.Broadcast();
	return true;
}
bool UInventoryComponent::RemoveItem(FName Id,int32 Quantity){if(!GetOwner()->HasAuthority()||Quantity<=0)return false;for(int32 i=0;i<Items.Num();++i)if(Items[i].ItemId==Id&&Items[i].Quantity>=Quantity){Items[i].Quantity-=Quantity;if(Items[i].Quantity==0)Items.RemoveAt(i);OnInventoryChanged.Broadcast();return true;}return false;}
bool UInventoryComponent::UpgradeCapacity(int32 NewMaxSlots){if(!GetOwner()->HasAuthority()||NewMaxSlots<=MaxSlots)return false;MaxSlots=FMath::Clamp(NewMaxSlots,6,20);OnInventoryChanged.Broadcast();return true;}
void UInventoryComponent::OnRep_Items(){OnInventoryChanged.Broadcast();}
void UInventoryComponent::OnRep_Capacity(){OnInventoryChanged.Broadcast();}
void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(UInventoryComponent,Items);DOREPLIFETIME(UInventoryComponent,MaxSlots);DOREPLIFETIME(UInventoryComponent,MaxWeight);}
