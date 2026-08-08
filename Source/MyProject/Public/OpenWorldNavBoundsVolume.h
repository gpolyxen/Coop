#pragma once

#include "CoreMinimal.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "OpenWorldNavBoundsVolume.generated.h"

class UBoxComponent;

UCLASS()
class MYPROJECT_API AOpenWorldNavBoundsVolume : public ANavMeshBoundsVolume
{
	GENERATED_BODY()
public:
	AOpenWorldNavBoundsVolume(const FObjectInitializer& ObjectInitializer);
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UBoxComponent* WorldBoundsBox;
};
