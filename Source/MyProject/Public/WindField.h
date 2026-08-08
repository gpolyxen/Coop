#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindField.generated.h"
UCLASS(Blueprintable)
class MYPROJECT_API AWindField : public AActor
{
	GENERATED_BODY()
public:
	AWindField();
	UFUNCTION(BlueprintPure) FVector GetWindAtLocation(FVector Location, float Time) const;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector BaseWindCmPerSecond = FVector(0, 500, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float GustStrength = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float GustFrequency = .2f;
};
