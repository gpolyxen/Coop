#pragma once

#include "CoreMinimal.h"
#include "PickupActor.h"
#include "BackpackPickup.generated.h"

class UPointLightComponent;
class UTextRenderComponent;
class UBoxComponent;

UCLASS(Blueprintable)
class MYPROJECT_API ABackpackPickup : public APickupActor
{
	GENERATED_BODY()
public:
	ABackpackPickup();
	virtual bool TryPickup(APawn* Pawn)override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	void ConfigureCapacity(int32 InCapacity);
	UPROPERTY(ReplicatedUsing=OnRep_Capacity,EditAnywhere,BlueprintReadOnly,Category="Backpack")int32 Capacity=20;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* TopPocket;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* LeftStrap;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* RightStrap;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UPointLightComponent* MarkerLight;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UTextRenderComponent* MarkerText;
	/** Simple physics body used because the imported visual mesh has no reliable simple collision. */
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UBoxComponent* PhysicsBox;
private:
	UFUNCTION()void OnRep_Capacity();
	void RefreshAppearance();
};
