#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieSpitProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class MYPROJECT_API AZombieSpitProjectile : public AActor
{
	GENERATED_BODY()
public:
	AZombieSpitProjectile();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	UPROPERTY(VisibleAnywhere) USphereComponent* Collision;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Visual;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* TrailVisual;
	UPROPERTY(VisibleAnywhere) TArray<UStaticMeshComponent*> SlimeGlobules;
	UPROPERTY(VisibleAnywhere) UPointLightComponent* SlimeGlow;
	UPROPERTY(VisibleAnywhere) UProjectileMovementComponent* Movement;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Damage") float Damage=16.f;
private:
	UFUNCTION() void OnProjectileHit(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit);
	float VisualAge=0.f;
};
