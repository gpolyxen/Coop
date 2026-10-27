#include "BloodPoolActor.h"

#include "Components/SceneComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABloodPoolActor::ABloodPoolActor()
{
	PrimaryActorTick.bCanEverTick=true;bReplicates=false;
	SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("BloodRoot"));RootComponent=SceneRoot;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BloodMaterial(TEXT("/Game/ThirdPersonBP/VFX/Blood/M_BloodAtlas_Decal.M_BloodAtlas_Decal"));
	for(int32 Index=0;Index<5;++Index)
	{
		UDecalComponent* Decal=CreateDefaultSubobject<UDecalComponent>(*FString::Printf(TEXT("BloodPoolDecal_%02d"),Index));
		Decal->SetupAttachment(SceneRoot);Decal->SetVisibility(false);Decal->SetFadeScreenSize(.001f);Decal->DecalSize=FVector(5.f,4.f,4.f);
		if(BloodMaterial.Succeeded())Decal->SetDecalMaterial(BloodMaterial.Object);PoolDecals.Add(Decal);
	}
}

void ABloodPoolActor::MoveBelowCorpse(const FVector& CorpseLocation)
{
	FHitResult GroundHit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BloodPoolGround),false,this);
	if(TrackedCorpseMesh.IsValid()&&TrackedCorpseMesh->GetOwner())Query.AddIgnoredActor(TrackedCorpseMesh->GetOwner());
	const FVector Start=CorpseLocation+FVector(0.f,0.f,120.f);
	const FVector End=CorpseLocation-FVector(0.f,0.f,1400.f);
	if(GetWorld()&&GetWorld()->LineTraceSingleByChannel(GroundHit,Start,End,ECC_WorldStatic,Query)&&GroundHit.ImpactNormal.Z>.55f)
	{
		SetActorLocation(GroundHit.ImpactPoint+GroundHit.ImpactNormal*.8f);SetActorRotation(GroundHit.ImpactNormal.Rotation());
		FVector TangentX=FVector::CrossProduct(GroundHit.ImpactNormal,FVector::ForwardVector).GetSafeNormal();if(TangentX.IsNearlyZero())TangentX=FVector::RightVector;
		const FVector TangentY=FVector::CrossProduct(GroundHit.ImpactNormal,TangentX).GetSafeNormal();
		for(int32 Index=0;Index<PoolDecals.Num();++Index)if(UDecalComponent* Decal=PoolDecals[Index])
		{
			const float Angle=Index*2.39996f+FMath::FRandRange(-.25f,.25f),Radius=Index==0?0.f:FMath::FRandRange(18.f,62.f);
			Decal->SetWorldLocation(GetActorLocation()+TangentX*FMath::Cos(Angle)*Radius+TangentY*FMath::Sin(Angle)*Radius);
			FRotator Rotation=GroundHit.ImpactNormal.Rotation();Rotation.Roll=FMath::FRandRange(0.f,360.f);Decal->SetWorldRotation(Rotation);Decal->SetVisibility(true);
		}
	}
	else for(UDecalComponent* Decal:PoolDecals)if(Decal)Decal->SetVisibility(false);
}

void ABloodPoolActor::ActivatePool(USkeletalMeshComponent* CorpseMesh,const FVector& CorpseLocation)
{
	TrackedCorpseMesh=CorpseMesh;for(UDecalComponent* Decal:PoolDecals)if(Decal)Decal->SetVisibility(false);MoveBelowCorpse(CorpseLocation);TargetDecalSizes.Reset();
	for(int32 Index=0;Index<PoolDecals.Num();++Index)
	{
		const int32 AtlasIndex=FMath::RandRange(0,7);const float Width=Index==0?FMath::FRandRange(75.f,105.f):FMath::FRandRange(28.f,72.f);
		TargetDecalSizes.Add(FVector(6.f,Width,FMath::FRandRange(Width*.45f,Width*.9f)));
		if(UMaterialInstanceDynamic* Material=PoolDecals[Index]->CreateDynamicMaterialInstance())Material->SetVectorParameterValue(TEXT("AtlasOffset"),FLinearColor((AtlasIndex%4)*.25f,(AtlasIndex/4)*.25f,0.f,0.f));
	}
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
	if(Age<9.f)for(int32 Index=0;Index<PoolDecals.Num()&&TargetDecalSizes.IsValidIndex(Index);++Index)
		PoolDecals[Index]->DecalSize=FMath::VInterpTo(PoolDecals[Index]->DecalSize,TargetDecalSizes[Index],DeltaSeconds,Index==0?.36f:.2f);
}
