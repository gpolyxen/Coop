#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BallisticProjectile.generated.h"
class USphereComponent; class UProjectileMovementComponent;class UNiagaraComponent;
UCLASS()
class MYPROJECT_API ABallisticProjectile : public AActor
{
	GENERATED_BODY()
public:
	ABallisticProjectile(); virtual void Tick(float DeltaSeconds) override;
	void InitializeProjectile(float InDamage,float InDrag,float InWind,float InGravityScale,float InLifeSeconds,AController* InInstigator);
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) USphereComponent* Collision;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) UProjectileMovementComponent* Movement;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UNiagaraComponent* Tracer;
protected:
	UFUNCTION() void OnProjectileHit(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit);
	float Damage=30.f,Drag=.25f,WindInfluence=1.f; TWeakObjectPtr<AController> DamageInstigator;
};
