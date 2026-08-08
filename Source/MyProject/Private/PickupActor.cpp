#include "PickupActor.h"
#include "InventoryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
APickupActor::APickupActor(){bReplicates=true;SetReplicateMovement(true);Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));RootComponent=Mesh;Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Mesh->SetSimulatePhysics(true);Mesh->SetEnableGravity(true);InteractionRange=CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));InteractionRange->SetupAttachment(RootComponent);InteractionRange->SetSphereRadius(140.f);InteractionRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));}
bool APickupActor::TryPickup(APawn* P){if(!HasAuthority()||!P||!InteractionRange->IsOverlappingActor(P))return false;if(UInventoryComponent* I=P->FindComponentByClass<UInventoryComponent>()){if(I->AddItem(ItemId,Quantity)){Destroy();return true;}}return false;}
