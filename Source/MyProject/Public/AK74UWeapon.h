#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "AK74UWeapon.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class MYPROJECT_API AAK74UWeapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	AAK74UWeapon();

	// Lightweight gun-only mesh. The downloaded AK74U first-person animation
	// package is deliberately not used; the established character rig holds it.
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="AK74U")UStaticMeshComponent* WorldGunMesh;
};
