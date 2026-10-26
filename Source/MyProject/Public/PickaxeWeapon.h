#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "PickaxeWeapon.generated.h"

class UStaticMeshComponent;

UCLASS()
class MYPROJECT_API APickaxeWeapon : public AWeaponBase
{
	GENERATED_BODY()
public:
	APickaxeWeapon();
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Handle;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Head;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Point;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* BackPoint;
};
