#pragma once
#include "CoreMinimal.h"
#include "PickupActor.h"
#include "WeaponPickup.generated.h"
class AWeaponBase;
UCLASS()
class MYPROJECT_API AWeaponPickup:public APickupActor
{
	GENERATED_BODY()
public:AWeaponPickup();virtual bool TryPickup(APawn* Pawn)override;void ConfigureWeaponClass(TSubclassOf<AWeaponBase> NewClass);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	UPROPERTY(ReplicatedUsing=OnRep_WeaponClass,EditAnywhere,BlueprintReadOnly)TSubclassOf<AWeaponBase> WeaponClass;
private:UFUNCTION()void OnRep_WeaponClass();
};
