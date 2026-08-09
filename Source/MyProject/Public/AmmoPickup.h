#pragma once

#include "PickupActor.h"
#include "AmmoPickup.generated.h"

class UPointLightComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class MYPROJECT_API AAmmoPickup : public APickupActor
{
	GENERATED_BODY()
public:
	AAmmoPickup();
	virtual bool TryPickup(APawn* Pawn)override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UPointLightComponent* MarkerLight;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UTextRenderComponent* MarkerText;
};
