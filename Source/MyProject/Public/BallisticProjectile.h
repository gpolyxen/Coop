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
	void InitializeProjectile(float InDamage,float InHeadshotMultiplier,float InLimbMultiplier,float InDrag,float InWind,float InGravityScale,float InLifeSeconds,AController* InInstigator);
	void SetCloseRangeHitCorrection(AActor* InTarget,FName InBoneName,const FVector& InImpactPoint);
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) USphereComponent* Collision;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly) UProjectileMovementComponent* Movement;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UNiagaraComponent* Tracer;
protected:
	UFUNCTION() void OnProjectileHit(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit);
	float Damage=30.f,HeadshotMultiplier=3.f,LimbMultiplier=.55f,Drag=.25f,WindInfluence=1.f; TWeakObjectPtr<AController> DamageInstigator;
	TWeakObjectPtr<AActor> CorrectedTarget;
	FName CorrectedBone=NAME_None;
	FVector CorrectedImpactPoint=FVector::ZeroVector;
};
