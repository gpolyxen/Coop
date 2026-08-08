#include "BallisticProjectile.h"
#include "WindField.h"
#include "HealthArmorComponent.h"
#include "ZombieCharacter.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

ABallisticProjectile::ABallisticProjectile(){PrimaryActorTick.bCanEverTick=true;bReplicates=true;SetReplicateMovement(true);Collision=CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));RootComponent=Collision;Collision->InitSphereRadius(3.f);Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));Collision->OnComponentHit.AddDynamic(this,&ABallisticProjectile::OnProjectileHit);Tracer=CreateDefaultSubobject<UNiagaraComponent>(TEXT("Tracer"));Tracer->SetupAttachment(RootComponent);static ConstructorHelpers::FObjectFinder<UNiagaraSystem>Trail(TEXT("/Game/sA_ShootingVfxPack/FX/NiagaraSystems/NS_BulletTrail_1.NS_BulletTrail_1"));if(Trail.Succeeded())Tracer->SetAsset(Trail.Object);Movement=CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));Movement->ProjectileGravityScale=1.f;Movement->bRotationFollowsVelocity=true;Movement->bShouldBounce=false;InitialLifeSpan=12.f;}
void ABallisticProjectile::InitializeProjectile(float D,float Dr,float W,AController* I){Damage=D;Drag=Dr;WindInfluence=W;DamageInstigator=I;}
void ABallisticProjectile::Tick(float Dt){Super::Tick(Dt);const FVector Current=GetActorLocation();if(!PreviousLocation.IsNearlyZero())DrawDebugLine(GetWorld(),PreviousLocation,Current,FColor(255,190,30),false,.12f,0,2.5f);PreviousLocation=Current;if(!HasAuthority())return;FVector Wind=FVector::ZeroVector;for(TActorIterator<AWindField>It(GetWorld());It;++It){Wind=It->GetWindAtLocation(GetActorLocation(),GetWorld()->GetTimeSeconds());break;}const FVector Relative=Movement->Velocity-Wind*WindInfluence;Movement->Velocity+=(-Relative*Drag*.01f+Wind*WindInfluence*.05f)*Dt;}
void ABallisticProjectile::OnProjectileHit(UPrimitiveComponent*,AActor* Other,UPrimitiveComponent*,FVector,const FHitResult& Hit){if(!HasAuthority()||!Other||Other==GetOwner())return;if(Cast<AZombieCharacter>(Other))UGameplayStatics::ApplyPointDamage(Other,Damage,Movement->Velocity.GetSafeNormal(),Hit,DamageInstigator.Get(),this,nullptr);else if(UHealthArmorComponent* H=Other->FindComponentByClass<UHealthArmorComponent>())H->ApplyDamage(Damage,DamageInstigator.Get(),this);else UGameplayStatics::ApplyPointDamage(Other,Damage,Movement->Velocity.GetSafeNormal(),Hit,DamageInstigator.Get(),this,nullptr);Destroy();}
