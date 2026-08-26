#include "BloodPoolActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABloodPoolActor::ABloodPoolActor()
{
	PrimaryActorTick.bCanEverTick=true;
	bReplicates=false;
	PoolMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BloodPool"));
	RootComponent=PoolMesh;
	PoolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoolMesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if(Sphere.Succeeded())PoolMesh->SetStaticMesh(Sphere.Object);
	if(BaseMaterial.Succeeded())PoolMesh->SetMaterial(0,BaseMaterial.Object);
	PoolMesh->SetRelativeScale3D(FVector(.08f,.08f,.012f));
}

void ABloodPoolActor::MoveBelowCorpse(const FVector& CorpseLocation)
{
	FHitResult GroundHit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BloodPoolGround),false,this);
	if(TrackedCorpseMesh.IsValid()&&TrackedCorpseMesh->GetOwner())Query.AddIgnoredActor(TrackedCorpseMesh->GetOwner());
	const FVector Start=CorpseLocation+FVector(0.f,0.f,90.f);
	const FVector End=CorpseLocation-FVector(0.f,0.f,240.f);
	const FVector Location=GetWorld()&&GetWorld()->LineTraceSingleByChannel(GroundHit,Start,End,ECC_Visibility,Query)
		?GroundHit.ImpactPoint+GroundHit.ImpactNormal*1.2f:CorpseLocation-FVector(0.f,0.f,88.f);
	SetActorLocation(Location);
}

void ABloodPoolActor::ActivatePool(USkeletalMeshComponent* CorpseMesh,const FVector& CorpseLocation)
{
	TrackedCorpseMesh=CorpseMesh;
	MoveBelowCorpse(CorpseLocation);
	TargetScale=FVector(FMath::FRandRange(1.25f,1.8f),FMath::FRandRange(1.0f,1.55f),.018f);
	if(UMaterialInstanceDynamic* Material=PoolMesh->CreateAndSetMaterialInstanceDynamic(0))
		Material->SetVectorParameterValue(TEXT("Color"),FLinearColor(.12f,.001f,.001f,1.f));
	SetLifeSpan(55.f);
}

void ABloodPoolActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age+=DeltaSeconds;
	// Follow the ragdoll while it falls or is thrown. Once it has settled, leave
	// the pool fixed on the floor instead of sliding forever with the corpse.
	if(Age<4.5f&&TrackedCorpseMesh.IsValid())
	{
		const FBoxSphereBounds Bounds=TrackedCorpseMesh->Bounds;
		MoveBelowCorpse(Bounds.Origin);
	}
	else TrackedCorpseMesh.Reset();
	if(Age<9.f)PoolMesh->SetRelativeScale3D(FMath::VInterpTo(PoolMesh->GetRelativeScale3D(),TargetScale,DeltaSeconds,.42f));
}
