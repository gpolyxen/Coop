#pragma once

#include "PickupActor.h"
#include "AmmoPickup.generated.h"

UCLASS(Blueprintable)
class MYPROJECT_API AAmmoPickup : public APickupActor
{
	GENERATED_BODY()
public:
	AAmmoPickup();
	virtual bool TryPickup(APawn* Pawn)override;
};
