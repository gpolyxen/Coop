#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaveBed.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class MYPROJECT_API ASaveBed : public AActor
{
	GENERATED_BODY()
public:
	ASaveBed();
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)USceneComponent* SceneRoot;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Frame;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Mattress;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Headboard;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UPointLightComponent* SaveLight;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UTextRenderComponent* SaveText;
};
