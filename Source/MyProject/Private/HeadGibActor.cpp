#include "HeadGibActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHeadGibActor::AHeadGibActor()
{
	PrimaryActorTick.bCanEverTick=false;
	bReplicates=false;
	GibMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DetachedHead"));
	RootComponent=GibMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if(Sphere.Succeeded())GibMesh->SetStaticMesh(Sphere.Object);
	if(FallbackMaterial.Succeeded())GibMesh->SetMaterial(0,FallbackMaterial.Object);
	GibMesh->SetRelativeScale3D(FVector(.22f,.18f,.25f));
	GibMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	GibMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GibMesh->SetSimulatePhysics(true);
	GibMesh->SetEnableGravity(true);
	GibMesh->SetLinearDamping(.35f);
	GibMesh->SetAngularDamping(.25f);
}

void AHeadGibActor::InitializeGib(UMaterialInterface* SourceMaterial,const FVector& Impulse)
{
	if(SourceMaterial)GibMesh->SetMaterial(0,SourceMaterial);
	GibMesh->AddImpulse(Impulse+FVector::UpVector*16000.f,NAME_None,true);
	GibMesh->AddAngularImpulseInRadians(FMath::VRand()*18.f,NAME_None,true);
	SetLifeSpan(12.f);
}
