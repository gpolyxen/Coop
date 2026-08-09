#include "BallisticProjectile.h"
#include "WindField.h"
#include "HealthArmorComponent.h"
#include "ZombieCharacter.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

ABallisticProjectile::ABallisticProjectile(){PrimaryActorTick.bCanEverTick=true;bReplicates=true;SetReplicateMovement(true);Collision=CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));RootComponent=Collision;Collision->InitSphereRadius(3.f);Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));Collision->OnComponentHit.AddDynamic(this,&ABallisticProjectile::OnProjectileHit);Tracer=CreateDefaultSubobject<UNiagaraComponent>(TEXT("Tracer"));Tracer->SetupAttachment(RootComponent);static ConstructorHelpers::FObjectFinder<UNiagaraSystem>Trail(TEXT("/Game/sA_ShootingVfxPack/FX/NiagaraSystems/NS_BulletTrail_1.NS_BulletTrail_1"));if(Trail.Succeeded())Tracer->SetAsset(Trail.Object);Movement=CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));Movement->ProjectileGravityScale=1.f;Movement->bRotationFollowsVelocity=true;Movement->bShouldBounce=false;InitialLifeSpan=12.f;}
void ABallisticProjectile::InitializeProjectile(float D,float InHeadshotMultiplier,float InLimbMultiplier,float Dr,float W,float GravityScale,float LifeSeconds,AController* I)
{
	Damage=D;
	HeadshotMultiplier=FMath::Max(1.f,InHeadshotMultiplier);
	LimbMultiplier=FMath::Clamp(InLimbMultiplier,0.f,1.f);
	Drag=Dr;
	WindInfluence=W;
	DamageInstigator=I;
	Movement->ProjectileGravityScale=FMath::Max(0.f,GravityScale);
	SetLifeSpan(FMath::Max(1.f,LifeSeconds));
	// A physical bullet can start inside the shooter's capsule or third-person
	// hands. Ignore the owning character before the first movement sweep so the
	// projectile does not stop on the player who fired it.
	if(Collision)
	{
		if(AActor* OwningActor=GetOwner())Collision->IgnoreActorWhenMoving(OwningActor,true);
		if(APawn* InstigatorPawn=GetInstigator())Collision->IgnoreActorWhenMoving(InstigatorPawn,true);
	}
}
void ABallisticProjectile::SetCloseRangeHitCorrection(AActor* InTarget,FName InBoneName,const FVector& InImpactPoint)
{
	CorrectedTarget=InTarget;
	CorrectedBone=InBoneName;
	CorrectedImpactPoint=InImpactPoint;
}
void ABallisticProjectile::Tick(float Dt){Super::Tick(Dt);if(!HasAuthority())return;FVector Wind=FVector::ZeroVector;for(TActorIterator<AWindField>It(GetWorld());It;++It){Wind=It->GetWindAtLocation(GetActorLocation(),GetWorld()->GetTimeSeconds());break;}const FVector RelativeVelocity=Movement->Velocity-Wind;const FVector DragAcceleration=-RelativeVelocity*FMath::Clamp(Drag*.0125f,0.f,.25f);const FVector WindAcceleration=Wind*WindInfluence*.35f;Movement->Velocity+=(DragAcceleration+WindAcceleration)*Dt;}
void ABallisticProjectile::OnProjectileHit(UPrimitiveComponent*,AActor* Other,UPrimitiveComponent*,FVector,const FHitResult& Hit)
{
	if(!HasAuthority()||!Other)return;
	if(Other==GetOwner()||Other==GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(Other,true);
		return;
	}
	FHitResult DamageHit=Hit;
	if(Other==CorrectedTarget.Get()&&!CorrectedBone.IsNone())
	{
		DamageHit.BoneName=CorrectedBone;
		DamageHit.ImpactPoint=CorrectedImpactPoint;
		DamageHit.Location=CorrectedImpactPoint;
	}
	float ZoneDamage=Damage;
	const FString HitBoneName=DamageHit.BoneName.ToString().ToLower();
	if(HitBoneName.Contains(TEXT("head")))ZoneDamage*=HeadshotMultiplier;
	else
	{
		static const TCHAR* LimbTokens[]={TEXT("arm"),TEXT("hand"),TEXT("finger"),TEXT("thumb"),TEXT("leg"),TEXT("upleg"),TEXT("foot"),TEXT("toe"),TEXT("thigh"),TEXT("calf")};
		for(const TCHAR* Token:LimbTokens)if(HitBoneName.Contains(Token)){ZoneDamage*=LimbMultiplier;break;}
	}
	if(Cast<AZombieCharacter>(Other))
		UGameplayStatics::ApplyPointDamage(Other,ZoneDamage,Movement->Velocity.GetSafeNormal(),DamageHit,DamageInstigator.Get(),this,nullptr);
	else if(UHealthArmorComponent* H=Other->FindComponentByClass<UHealthArmorComponent>())
		H->ApplyDamage(Damage,DamageInstigator.Get(),this);
	else UGameplayStatics::ApplyPointDamage(Other,Damage,Movement->Velocity.GetSafeNormal(),DamageHit,DamageInstigator.Get(),this,nullptr);
	Destroy();
}
