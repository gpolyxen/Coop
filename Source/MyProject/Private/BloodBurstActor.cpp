#include "BloodBurstActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABloodBurstActor::ABloodBurstActor()
{
	PrimaryActorTick.bCanEverTick=true;
	bReplicates=false;
	SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("BloodRoot"));
	RootComponent=SceneRoot;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	for(int32 Index=0;Index<34;++Index)
	{
		UStaticMeshComponent* Drop=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("BloodDrop_%02d"),Index));
		Drop->SetupAttachment(SceneRoot);
		Drop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Drop->SetCastShadow(false);
		Drop->SetVisibility(false);
		if(Sphere.Succeeded())Drop->SetStaticMesh(Sphere.Object);
		if(BaseMaterial.Succeeded())Drop->SetMaterial(0,BaseMaterial.Object);
		Droplets.Add(Drop);
	}
}

void ABloodBurstActor::ActivateBurst(const FVector& ShotDirection,bool bFountain)
{
	bActive=true;Age=0.f;Velocities.Reset();
	SetLifeSpan(4.f);
	const FVector Forward=ShotDirection.IsNearlyZero()?GetActorForwardVector():ShotDirection.GetSafeNormal();
	const int32 VisibleCount=bFountain?34:16;
	for(int32 Index=0;Index<Droplets.Num();++Index)
	{
		UStaticMeshComponent* Drop=Droplets[Index];
		const bool bVisible=Index<VisibleCount;
		Drop->SetVisibility(bVisible);
		if(!bVisible){Velocities.Add(FVector::ZeroVector);continue;}
		UMaterialInstanceDynamic* Material=Drop->CreateAndSetMaterialInstanceDynamic(0);
		if(Material)
		{
			const FLinearColor BloodColor=Index%3==0?FLinearColor(.16f,.002f,.002f):FLinearColor(.42f,.006f,.004f);
			Material->SetVectorParameterValue(TEXT("Color"),BloodColor);
		}
		const float Scale=bFountain?FMath::FRandRange(.032f,.085f):FMath::FRandRange(.024f,.058f);
		Drop->SetWorldScale3D(FVector(Scale,FMath::FRandRange(Scale*.35f,Scale),Scale));
		Drop->SetRelativeLocation(FMath::VRand()*FMath::FRandRange(0.f,8.f));
		FVector Velocity;
		if(bFountain)
			Velocity=FVector(Forward.X,Forward.Y,0.f)*FMath::FRandRange(80.f,280.f)+FVector::UpVector*FMath::FRandRange(420.f,820.f)+FMath::VRand()*180.f;
		else Velocity=Forward*FMath::FRandRange(180.f,520.f)+FMath::VRand()*260.f+FVector::UpVector*FMath::FRandRange(20.f,180.f);
		Velocities.Add(Velocity);
	}
}

void ABloodBurstActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if(!bActive)return;
	Age+=DeltaSeconds;
	for(int32 Index=0;Index<Droplets.Num()&&Velocities.IsValidIndex(Index);++Index)
	{
		if(!Droplets[Index]->IsVisible())continue;
		Velocities[Index].Z-=980.f*DeltaSeconds;
		Droplets[Index]->AddWorldOffset(Velocities[Index]*DeltaSeconds,false);
		if(Age>1.65f)Droplets[Index]->SetVisibility(false);
	}
}
