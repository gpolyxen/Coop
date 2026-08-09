#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "P9Weapon.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS(Blueprintable)
class MYPROJECT_API AP9Weapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	AP9Weapon();
	virtual FVector GetMuzzleLocation()const override;

	// Third-person/world representation. The owning player sees the dedicated
	// FPS rig instead; other players see this component in sktGun/skt_back_1.
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="P9")UStaticMeshComponent* WorldPistolMesh;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="P9")USceneComponent* MuzzlePoint;
};
