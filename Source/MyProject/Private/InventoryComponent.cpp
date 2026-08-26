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
	if(A==B&&!A.IsNone()){AQ+=BQ;BQ=0;B=NAME_None;}
	const FItemDefinition* AD=FindDefinition(A);const FItemDefinition* BD=FindDefinition(B);
	const float AddedWeight=(AD?AD->Weight:1.f)*AQ+(BD?BD->Weight:1.f)*BQ;
	if(CurrentWeight()+AddedWeight>MaxWeight)return false;
	const int32 AMaxStack=OverrideMaxStack>0?OverrideMaxStack:((A==TEXT("Wood")||A==TEXT("Rope"))?999:(AD?AD->MaxStack:99));const int32 BMaxStack=OverrideMaxStack>0?OverrideMaxStack:((B==TEXT("Wood")||B==TEXT("Rope"))?999:(BD?BD->MaxStack:99));
	if(!bAllowMultipleStacks&&(GetQuantity(A)+AQ>AMaxStack||GetQuantity(B)+BQ>BMaxStack))return false;
	auto NeededFor=[this](FName Id,int32 Quantity,int32 MaxStack){if(Quantity<=0||Id.IsNone())return 0;int32 Free=0;for(const FInventoryEntry& Entry:Items)if(Entry.ItemId==Id)Free+=FMath::Max(0,MaxStack-Entry.Quantity);return FMath::Max(0,FMath::DivideAndRoundUp(FMath::Max(0,Quantity-Free),MaxStack));};
	int32 NeededSlots=NeededFor(A,AQ,AMaxStack);if(B!=A)NeededSlots+=NeededFor(B,BQ,BMaxStack);
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
	if(!CanAddItems(Id,Quantity,NAME_None,0))return false;
	const FItemDefinition* Definition=FindDefinition(Id);
	const float UnitWeight=Definition?Definition->Weight:1.f;
	const int32 MaxStack=OverrideMaxStack>0?OverrideMaxStack:((Id==TEXT("Wood")||Id==TEXT("Rope"))?999:(Definition?Definition->MaxStack:99));
	if(CurrentWeight()+UnitWeight*Quantity>MaxWeight)return false;
	for(FInventoryEntry& Entry:Items)
		if(Entry.ItemId==Id&&Entry.Quantity<MaxStack)
		{
			const int32 Added=FMath::Min(Quantity,MaxStack-Entry.Quantity);Entry.Quantity+=Added;Quantity-=Added;if(Quantity<=0){OnInventoryChanged.Broadcast();return true;}
		}
	if(!bAllowMultipleStacks&&Quantity>MaxStack)return false;
	while(Quantity>0){if(Items.Num()>=MaxSlots)return false;FInventoryEntry Entry;Entry.ItemId=Id;Entry.Quantity=FMath::Min(Quantity,MaxStack);Items.Add(Entry);Quantity-=Entry.Quantity;if(!bAllowMultipleStacks&&Quantity>0)return false;}
	OnInventoryChanged.Broadcast();
	return true;
}
int32 UInventoryComponent::AddItemPartial(FName Id,int32 RequestedQuantity)
{
	if(!GetOwner()->HasAuthority()||RequestedQuantity<=0||Id.IsNone())return 0;
	int32 Low=0,High=RequestedQuantity;
	while(Low<High)
	{
		const int32 Middle=Low+(High-Low+1)/2;
		if(CanAddItems(Id,Middle,NAME_None,0))Low=Middle;else High=Middle-1;
	}
	return Low>0&&AddItem(Id,Low)?Low:0;
}
bool UInventoryComponent::RemoveItem(FName Id,int32 Quantity){if(!GetOwner()->HasAuthority()||Quantity<=0||GetQuantity(Id)<Quantity)return false;for(int32 i=Items.Num()-1;i>=0&&Quantity>0;--i)if(Items[i].ItemId==Id){const int32 Removed=FMath::Min(Quantity,Items[i].Quantity);Items[i].Quantity-=Removed;Quantity-=Removed;if(Items[i].Quantity==0)Items.RemoveAt(i);}OnInventoryChanged.Broadcast();return true;}
bool UInventoryComponent::UpgradeCapacity(int32 NewMaxSlots)
{
	if(!GetOwner()->HasAuthority()||NewMaxSlots<=MaxSlots)return false;
	MaxSlots=FMath::Clamp(NewMaxSlots,6,20);
	MaxWeight=FMath::Max(MaxWeight,MaxSlots*100.f);
	OnInventoryChanged.Broadcast();
	return true;
}
void UInventoryComponent::OnRep_Items(){OnInventoryChanged.Broadcast();}
void UInventoryComponent::OnRep_Capacity(){OnInventoryChanged.Broadcast();}
void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(UInventoryComponent,Items);DOREPLIFETIME(UInventoryComponent,MaxSlots);DOREPLIFETIME(UInventoryComponent,MaxWeight);}
