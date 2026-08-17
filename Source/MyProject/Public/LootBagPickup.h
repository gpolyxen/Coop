#pragma once

#include "CoreMinimal.h"
#include "PickupActor.h"
#include "LootBagPickup.generated.h"

UCLASS(Blueprintable)
class MYPROJECT_API ALootBagPickup : public APickupActor
{
	GENERATED_BODY()
public:
	ALootBagPickup();
	virtual void BeginPlay() override;
	virtual bool TryPickup(APawn* Pawn) override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Loot") UStaticMeshComponent* BagTop;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Loot") UStaticMeshComponent* RopeTie;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Loot") UStaticMeshComponent* StickA;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Loot") UStaticMeshComponent* StickB;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Loot") class UTextRenderComponent* PickupText;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Loot") int32 MinWood=1;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Loot") int32 MaxWood=4;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Loot") int32 MinRope=1;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Loot") int32 MaxRope=3;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Loot",meta=(ClampMin="0.0",ClampMax="1.0")) float MedicineChance=.32f;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Loot",meta=(ClampMin="0.0",ClampMax="1.0")) float BandageChance=.58f;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Loot") int32 MaxBandages=2;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Loot",meta=(ClampMin="0.0",ClampMax="1.0")) float LeatherChance=.42f;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Loot") int32 MaxLeather=2;
};
