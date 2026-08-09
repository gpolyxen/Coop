#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HeadGibActor.generated.h"

class UMaterialInterface;
class UStaticMeshComponent;

UCLASS(NotBlueprintable)
class MYPROJECT_API AHeadGibActor : public AActor
{
	GENERATED_BODY()
public:
	AHeadGibActor();
	void InitializeGib(UMaterialInterface* SourceMaterial,const FVector& Impulse);
private:
	UPROPERTY()UStaticMeshComponent* GibMesh=nullptr;
};
