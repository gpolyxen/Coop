#pragma once
#include "CoreMinimal.h"
#include "PickupActor.h"
#include "WeaponPickup.generated.h"
class AWeaponBase;
class UBoxComponent;
UCLASS()
class MYPROJECT_API AWeaponPickup:public APickupActor
{
	GENERATED_BODY()
public:AWeaponPickup();virtual bool TryPickup(APawn* Pawn)override;void ConfigureWeaponClass(TSubclassOf<AWeaponBase> NewClass);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	UPROPERTY(ReplicatedUsing=OnRep_WeaponClass,EditAnywhere,BlueprintReadOnly)TSubclassOf<AWeaponBase> WeaponClass;
	// Imported weapon meshes are visual only. This stable primitive prevents a
	// pickup from falling through the level when its asset has no simple collision.
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UBoxComponent* PhysicsBounds;
private:UFUNCTION()void OnRep_WeaponClass();
};
