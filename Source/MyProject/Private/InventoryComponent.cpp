#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent() { SetIsReplicatedByDefault(true); }
const FItemDefinition* UInventoryComponent::FindDefinition(FName Id) const { return ItemDefinitions ? ItemDefinitions->FindRow<FItemDefinition>(Id, TEXT("Inventory")) : nullptr; }
int32 UInventoryComponent::GetQuantity(FName Id) const { for (const FInventoryEntry& E : Items) if (E.ItemId == Id) return E.Quantity; return 0; }
float UInventoryComponent::CurrentWeight() const { float Sum=0; for(const FInventoryEntry& E:Items) if(const FItemDefinition* D=FindDefinition(E.ItemId)) Sum += D->Weight*E.Quantity; return Sum; }
bool UInventoryComponent::AddItem(FName Id, int32 Quantity)
{
	if (!GetOwner()->HasAuthority() || Quantity <= 0) return false; const FItemDefinition* D=FindDefinition(Id); if(!D || CurrentWeight()+D->Weight*Quantity>MaxWeight) return false;
	for(FInventoryEntry& E:Items) if(E.ItemId==Id && E.Quantity+Quantity<=D->MaxStack){E.Quantity+=Quantity;OnInventoryChanged.Broadcast();return true;}
	if(Items.Num()>=MaxSlots || Quantity>D->MaxStack) return false; FInventoryEntry E;E.ItemId=Id;E.Quantity=Quantity;Items.Add(E);OnInventoryChanged.Broadcast();return true;
}
bool UInventoryComponent::RemoveItem(FName Id,int32 Quantity){if(!GetOwner()->HasAuthority()||Quantity<=0)return false;for(int32 i=0;i<Items.Num();++i)if(Items[i].ItemId==Id&&Items[i].Quantity>=Quantity){Items[i].Quantity-=Quantity;if(Items[i].Quantity==0)Items.RemoveAt(i);OnInventoryChanged.Broadcast();return true;}return false;}
void UInventoryComponent::OnRep_Items(){OnInventoryChanged.Broadcast();}
void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(UInventoryComponent,Items);}
