#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupActor.generated.h"
class USphereComponent; class UStaticMeshComponent;
UCLASS(Blueprintable)
class MYPROJECT_API APickupActor:public AActor
{
	GENERATED_BODY()
public:
	APickupActor();
	UFUNCTION(BlueprintCallable) virtual bool TryPickup(APawn* Pawn);
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) UStaticMeshComponent* Mesh;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) USphereComponent* InteractionRange;
	UPROPERTY(EditAnywhere,BlueprintReadOnly) FName ItemId;
	UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Quantity=1;
};
