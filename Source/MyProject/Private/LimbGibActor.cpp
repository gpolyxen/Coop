#include "LimbGibActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALimbGibActor::ALimbGibActor()
{
	PrimaryActorTick.bCanEverTick=false;
	GibMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DetachedLimb"));
	RootComponent=GibMesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if(Cylinder.Succeeded())GibMesh->SetStaticMesh(Cylinder.Object);
	GibMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	GibMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GibMesh->SetSimulatePhysics(true);
	GibMesh->SetEnableGravity(true);
	GibMesh->SetLinearDamping(.35f);
	GibMesh->SetAngularDamping(.25f);
}

void ALimbGibActor::InitializeGib(UMaterialInterface* Material,bool bLeg,const FVector& Impulse)
{
	if(Material)GibMesh->SetMaterial(0,Material);
	SetActorScale3D(bLeg?FVector(.16f,.16f,.62f):FVector(.13f,.13f,.48f));
	GibMesh->AddImpulse(Impulse,NAME_None,true);
	GibMesh->AddAngularImpulseInRadians(FVector(FMath::FRandRange(-9.f,9.f),FMath::FRandRange(-9.f,9.f),FMath::FRandRange(-9.f,9.f)),NAME_None,true);
	SetLifeSpan(18.f);
}
