#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BanditCharacter.generated.h"

class USkeletalMeshComponent;
class ABallisticProjectile;

/** Armed hostile that reuses the project's third-person mannequin presentation. */
UCLASS()
class MYPROJECT_API ABanditCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	ABanditCharacter();
	virtual void BeginPlay()override;
	virtual float TakeDamage(float DamageAmount,const FDamageEvent& DamageEvent,AController* EventInstigator,AActor* DamageCauser)override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	bool FireAt(AActor* Target);
	bool TryMelee(AActor* Target);
	UFUNCTION(BlueprintPure)bool IsDead()const{return bDead;}
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)USkeletalMeshComponent* WeaponMesh;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Bandit|Combat")TSubclassOf<ABallisticProjectile> ProjectileClass;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Bandit|Combat")float MaxHealth=140.f;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Bandit|Combat")float Health=140.f;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Bandit|Combat")bool bDead=false;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Bandit|Combat")float BulletDamage=16.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Bandit|Combat")float BulletSpeed=76000.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Bandit|Combat")float SpreadDegrees=1.15f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Bandit|Combat")float MeleeDamage=24.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Bandit|Combat")float MeleeCooldown=1.25f;
protected:
	UFUNCTION(NetMulticast,Reliable)void MulticastDie();
	double LastMeleeTime=-1000.;
};
