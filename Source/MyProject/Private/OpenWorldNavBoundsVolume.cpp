#include "OpenWorldNavBoundsVolume.h"
#include "Components/BoxComponent.h"

AOpenWorldNavBoundsVolume::AOpenWorldNavBoundsVolume(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
	WorldBoundsBox=CreateDefaultSubobject<UBoxComponent>(TEXT("OpenWorldBounds"));
	WorldBoundsBox->SetupAttachment(GetRootComponent());
	WorldBoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorldBoundsBox->SetBoxExtent(FVector(350000.f,350000.f,100000.f));
	WorldBoundsBox->SetHiddenInGame(true);
	WorldBoundsBox->SetCanEverAffectNavigation(false);
}
