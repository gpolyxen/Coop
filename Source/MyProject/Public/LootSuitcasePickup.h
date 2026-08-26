#pragma once

#include "CoreMinimal.h"
#include "LootBagPickup.h"
#include "LootSuitcasePickup.generated.h"

/** A larger, reusable loot container used inside generated ruins. */
UCLASS()
class MYPROJECT_API ALootSuitcasePickup : public ALootBagPickup
{
	GENERATED_BODY()
public:
	ALootSuitcasePickup();
	virtual void BeginPlay()override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Loot")UStaticMeshComponent* Handle;
};
