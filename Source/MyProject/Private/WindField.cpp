#include "WindField.h"
AWindField::AWindField(){PrimaryActorTick.bCanEverTick=false;bReplicates=true;}
FVector AWindField::GetWindAtLocation(FVector L,float T)const{const float N=FMath::PerlinNoise1D(T*GustFrequency+L.X*.0001f);return BaseWindCmPerSecond+BaseWindCmPerSecond.GetSafeNormal()*N*GustStrength;}
