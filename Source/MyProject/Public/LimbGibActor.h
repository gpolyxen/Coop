#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LimbGibActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class MYPROJECT_API ALimbGibActor : public AActor
{
	GENERATED_BODY()
public:
	ALimbGibActor();
	void InitializeGib(UMaterialInterface* Material,bool bLeg,const FVector& Impulse);
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* GibMesh;
};
