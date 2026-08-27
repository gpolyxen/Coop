#include "StorageChest.h"
#include "InventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "WeaponBase.h"
#include "KA47Rifle.h"
#include "P9Weapon.h"
#include "AK74UWeapon.h"
#include "SMG11Weapon.h"
#include "StarterRifle.h"
AStorageChest::AStorageChest()
{
	MaxStructureHealth=450.f;StructureHealth=450.f;bNeedsFoundationSupport=true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	Box=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestBox"));Box->SetupAttachment(SceneRoot);if(Cube.Succeeded())Box->SetStaticMesh(Cube.Object);if(Wood.Succeeded())Box->SetMaterial(0,Wood.Object);Box->SetRelativeLocation(FVector(0,0,38));Box->SetRelativeScale3D(FVector(.75f,1.15f,.55f));Box->SetCollisionProfileName(TEXT("BlockAll"));
	Lid=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestLid"));Lid->SetupAttachment(SceneRoot);if(Cube.Succeeded())Lid->SetStaticMesh(Cube.Object);if(Wood.Succeeded())Lid->SetMaterial(0,Wood.Object);Lid->SetRelativeLocation(FVector(0,0,75));Lid->SetRelativeScale3D(FVector(.82f,1.22f,.16f));Lid->SetCollisionProfileName(TEXT("BlockAll"));
	UseText=CreateDefaultSubobject<UTextRenderComponent>(TEXT("UseText"));UseText->SetupAttachment(SceneRoot);UseText->SetRelativeLocation(FVector(0,0,115));UseText->SetHorizontalAlignment(EHTA_Center);UseText->SetWorldSize(18.f);UseText->SetText(FText::FromString(TEXT("E  СУНДУК")));UseText->bAlwaysRenderAsText=true;
	Storage=CreateDefaultSubobject<UInventoryComponent>(TEXT("ChestInventory"));Storage->MaxSlots=20;Storage->MaxWeight=1000.f;Storage->OverrideMaxStack=100;Storage->bAllowMultipleStacks=true;
}
FName AStorageChest::GetStoredWeaponId(const AWeaponBase* Weapon)const
{
	if(!Weapon)return NAME_None;if(Weapon->IsA<AKA47Rifle>())return TEXT("Weapon_KA47");if(Weapon->IsA<AP9Weapon>())return TEXT("Weapon_P9");if(Weapon->IsA<AAK74UWeapon>())return TEXT("Weapon_AK74U");if(Weapon->IsA<ASMG11Weapon>())return TEXT("Weapon_SMG11");if(Weapon->IsA<AStarterRifle>())return TEXT("Weapon_AR4");return FName(*FString::Printf(TEXT("Weapon_%s"),*Weapon->GetClass()->GetName()));
}
AWeaponBase* AStorageChest::FindStoredWeapon(FName ItemId)const{for(AWeaponBase* Weapon:StoredWeapons)if(Weapon&&GetStoredWeaponId(Weapon)==ItemId)return Weapon;return nullptr;}
void AStorageChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AStorageChest,StoredWeapons);}
