#pragma once

#include "PickupActor.h"
#include "HealthPickup.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UBoxComponent;

UCLASS(Blueprintable)
class MYPROJECT_API AHealthPickup : public APickupActor
{
	GENERATED_BODY()
public:
	AHealthPickup();
	virtual void BeginPlay()override;
	virtual void Tick(float DeltaSeconds)override;
	virtual bool TryPickup(APawn* Pawn)override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UBoxComponent* PhysicsBody;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* CrossBar;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UPointLightComponent* MarkerLight;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UTextRenderComponent* MarkerText;
};
