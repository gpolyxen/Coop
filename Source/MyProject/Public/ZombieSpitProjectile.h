#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieSpitProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class MYPROJECT_API AZombieSpitProjectile : public AActor
{
	GENERATED_BODY()
public:
	AZombieSpitProjectile();
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere) USphereComponent* Collision;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Visual;
	UPROPERTY(VisibleAnywhere) UProjectileMovementComponent* Movement;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Damage") float Damage=16.f;
private:
	UFUNCTION() void OnProjectileHit(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit);
};
