#pragma once
#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "WoodAxeWeapon.generated.h"

class UStaticMeshComponent;

UCLASS()
class MYPROJECT_API AWoodAxeWeapon : public AWeaponBase
{
	GENERATED_BODY()
public:
	AWoodAxeWeapon();
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* AxeMesh;
};
