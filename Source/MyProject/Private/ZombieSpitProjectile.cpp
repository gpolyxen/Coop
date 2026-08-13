#include "ZombieSpitProjectile.h"

#include "HealthArmorComponent.h"
#include "ZombieCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AZombieSpitProjectile::AZombieSpitProjectile()
{
	bReplicates=true;
	SetReplicateMovement(true);
	InitialLifeSpan=6.f;
	Collision=CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	RootComponent=Collision;
	Collision->InitSphereRadius(11.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
	Collision->OnComponentHit.AddDynamic(this,&AZombieSpitProjectile::OnProjectileHit);
	Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if(Sphere.Succeeded())Visual->SetStaticMesh(Sphere.Object);
	Visual->SetRelativeScale3D(FVector(.20f,.20f,.20f));
	Movement=CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed=1350.f;
	Movement->MaxSpeed=1350.f;
	Movement->ProjectileGravityScale=.32f;
	Movement->bRotationFollowsVelocity=true;
}

void AZombieSpitProjectile::BeginPlay()
{
	Super::BeginPlay();
	Collision->IgnoreActorWhenMoving(GetOwner(),true);
	for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It)Collision->IgnoreActorWhenMoving(*It,true);
}

void AZombieSpitProjectile::OnProjectileHit(UPrimitiveComponent*,AActor* OtherActor,UPrimitiveComponent*,FVector,const FHitResult&)
{
	if(!HasAuthority()||!OtherActor||OtherActor==GetOwner())return;
	if(UHealthArmorComponent* Health=OtherActor->FindComponentByClass<UHealthArmorComponent>())
		Health->ApplyDamage(Damage,GetInstigatorController(),this);
	Destroy();
}
