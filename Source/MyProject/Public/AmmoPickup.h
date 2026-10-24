#pragma once

#include "PickupActor.h"
#include "AmmoPickup.generated.h"

class UPointLightComponent;
class UTextRenderComponent;
class UBoxComponent;

UCLASS(Blueprintable)
class MYPROJECT_API AAmmoPickup : public APickupActor
{
	GENERATED_BODY()
public:
	AAmmoPickup();
	virtual void BeginPlay()override;
	virtual void Tick(float DeltaSeconds)override;
	virtual bool TryPickup(APawn* Pawn)override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UBoxComponent* PhysicsBody;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Ammo")FName AmmoType=TEXT("AK74Ammo");
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UPointLightComponent* MarkerLight;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UTextRenderComponent* MarkerText;
};
