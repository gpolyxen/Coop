#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent() { SetIsReplicatedByDefault(true); }
const FItemDefinition* UInventoryComponent::FindDefinition(FName Id) const { return ItemDefinitions ? ItemDefinitions->FindRow<FItemDefinition>(Id, TEXT("Inventory")) : nullptr; }
int32 UInventoryComponent::GetQuantity(FName Id) const { int32 Total=0;for(const FInventoryEntry& E:Items)if(E.ItemId==Id)Total+=E.Quantity;return Total; }
float UInventoryComponent::CurrentWeight() const { float Sum=0; for(const FInventoryEntry& E:Items){const FItemDefinition* D=FindDefinition(E.ItemId);Sum+=(D?D->Weight:1.f)*E.Quantity;}return Sum; }
bool UInventoryComponent::HasItems(FName A,int32 AQ,FName B,int32 BQ)const{return GetQuantity(A)>=AQ&&GetQuantity(B)>=BQ;}
bool UInventoryComponent::CanAddItems(FName A,int32 AQ,FName B,int32 BQ)const
{
	if(AQ<0||BQ<0)return false;
	const FItemDefinition* AD=FindDefinition(A);const FItemDefinition* BD=FindDefinition(B);
	const float AddedWeight=(AD?AD->Weight:1.f)*AQ+(BD?BD->Weight:1.f)*BQ;
	if(CurrentWeight()+AddedWeight>MaxWeight)return false;
	const int32 AMaxStack=(A==TEXT("Wood")||A==TEXT("Rope"))?999:(AD?AD->MaxStack:99);const int32 BMaxStack=(B==TEXT("Wood")||B==TEXT("Rope"))?999:(BD?BD->MaxStack:99);
	if(GetQuantity(A)+AQ>AMaxStack||GetQuantity(B)+BQ>BMaxStack)return false;
	int32 NeededSlots=0;
	if(AQ>0&&GetQuantity(A)==0)++NeededSlots;
	if(BQ>0&&B!=A&&GetQuantity(B)==0)++NeededSlots;
	return Items.Num()+NeededSlots<=MaxSlots;
}
bool UInventoryComponent::ConsumeItems(FName A,int32 AQ,FName B,int32 BQ)
{
	if(!GetOwner()->HasAuthority()||!HasItems(A,AQ,B,BQ))return false;
	return RemoveItem(A,AQ)&&RemoveItem(B,BQ);
}
bool UInventoryComponent::AddItem(FName Id, int32 Quantity)
{
	if(!GetOwner()->HasAuthority()||Quantity<=0||Id.IsNone())return false;
	const FItemDefinition* Definition=FindDefinition(Id);
	const float UnitWeight=Definition?Definition->Weight:1.f;
	const int32 MaxStack=(Id==TEXT("Wood")||Id==TEXT("Rope"))?999:(Definition?Definition->MaxStack:99);
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
