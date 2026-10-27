#include "ZombieSpitProjectile.h"

#include "HealthArmorComponent.h"
#include "ZombieCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AZombieSpitProjectile::AZombieSpitProjectile()
{
	PrimaryActorTick.bCanEverTick=true;
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
	Visual->SetRelativeScale3D(FVector(.24f,.15f,.15f));
	TrailVisual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlimeTrail"));
	TrailVisual->SetupAttachment(Collision);
	TrailVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TrailVisual->SetCastShadow(false);
	if(Sphere.Succeeded())TrailVisual->SetStaticMesh(Sphere.Object);
	TrailVisual->SetRelativeLocation(FVector(-20.f,0.f,0.f));
	TrailVisual->SetRelativeScale3D(FVector(.28f,.055f,.055f));
	for(int32 Index=0;Index<6;++Index)
	{
		UStaticMeshComponent* Globule=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("SlimeGlobule_%02d"),Index));
		Globule->SetupAttachment(Collision);Globule->SetCollisionEnabled(ECollisionEnabled::NoCollision);Globule->SetCastShadow(false);
		if(Sphere.Succeeded())Globule->SetStaticMesh(Sphere.Object);
		Globule->SetRelativeLocation(FVector(-8.f-Index*4.f,(Index%2?1.f:-1.f)*(5.f+Index),((Index%3)-1)*5.f));
		Globule->SetRelativeScale3D(FVector(.055f+Index*.006f,.04f,.04f));
		SlimeGlobules.Add(Globule);
	}
	SlimeGlow=CreateDefaultSubobject<UPointLightComponent>(TEXT("SlimeGlow"));
	SlimeGlow->SetupAttachment(Collision);SlimeGlow->SetLightColor(FLinearColor(.18f,1.f,.015f));SlimeGlow->SetIntensity(900.f);SlimeGlow->SetAttenuationRadius(180.f);SlimeGlow->SetCastShadows(false);
	Movement=CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed=1350.f;
	Movement->MaxSpeed=1350.f;
	Movement->ProjectileGravityScale=.32f;
	Movement->bRotationFollowsVelocity=true;
}

void AZombieSpitProjectile::BeginPlay()
{
	Super::BeginPlay();
	const FLinearColor SlimeColor(.08f,.7f,.012f,1.f);
	TArray<UStaticMeshComponent*> Parts=SlimeGlobules;Parts.Add(Visual);Parts.Add(TrailVisual);
	for(UStaticMeshComponent* Part:Parts)if(Part)
		if(UMaterialInstanceDynamic* Material=Part->CreateAndSetMaterialInstanceDynamic(0))Material->SetVectorParameterValue(TEXT("Color"),SlimeColor);
	Collision->IgnoreActorWhenMoving(GetOwner(),true);
	for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It)Collision->IgnoreActorWhenMoving(*It,true);
}

void AZombieSpitProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);VisualAge+=DeltaSeconds;
	const float Pulse=1.f+FMath::Sin(VisualAge*15.f)*.09f;
	if(Visual)Visual->SetRelativeScale3D(FVector(.24f,.15f*Pulse,.15f*Pulse));
	if(TrailVisual)TrailVisual->SetRelativeScale3D(FVector(.25f+FMath::Sin(VisualAge*11.f)*.04f,.055f*Pulse,.055f*Pulse));
	for(int32 Index=0;Index<SlimeGlobules.Num();++Index)if(SlimeGlobules[Index])
	{
		FVector Location=SlimeGlobules[Index]->GetRelativeLocation();
		Location.Y+=FMath::Sin(VisualAge*(9.f+Index)+Index)*DeltaSeconds*12.f;
		Location.Z+=FMath::Cos(VisualAge*(8.f+Index)+Index)*DeltaSeconds*10.f;
		SlimeGlobules[Index]->SetRelativeLocation(Location);
	}
}

void AZombieSpitProjectile::OnProjectileHit(UPrimitiveComponent*,AActor* OtherActor,UPrimitiveComponent*,FVector,const FHitResult&)
{
	if(!HasAuthority()||!OtherActor||OtherActor==GetOwner())return;
	if(UHealthArmorComponent* Health=OtherActor->FindComponentByClass<UHealthArmorComponent>())
		Health->ApplyDamage(Damage,GetInstigatorController(),this);
	Destroy();
}
