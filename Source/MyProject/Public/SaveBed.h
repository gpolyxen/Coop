#pragma once

#include "CoreMinimal.h"
#include "BuildableStructure.h"
#include "SaveBed.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class MYPROJECT_API ASaveBed : public ABuildableStructure
{
	GENERATED_BODY()
public:
	ASaveBed();
	virtual void BeginPlay()override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Frame;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Mattress;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Headboard;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UPointLightComponent* SaveLight;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UTextRenderComponent* SaveText;
protected:virtual bool HasStructuralSupport()const override;
private:
	TWeakObjectPtr<class AWoodFloor> SupportingFloor;
	bool bRequiresFloorSupport=false;
};
