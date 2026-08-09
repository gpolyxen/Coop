#include "WindField.h"
#include "Components/SceneComponent.h"

AWindField::AWindField()
{
	PrimaryActorTick.bCanEverTick=false;
	bReplicates=true;
	RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
}
FVector AWindField::GetWindAtLocation(FVector L,float T)const{const float N=FMath::PerlinNoise1D(T*GustFrequency+L.X*.0001f);return BaseWindCmPerSecond+BaseWindCmPerSecond.GetSafeNormal()*N*GustStrength;}
