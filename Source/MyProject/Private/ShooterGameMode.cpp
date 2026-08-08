#include "ShooterGameMode.h"
#include "ShooterCharacter.h"
#include "ShooterHUD.h"
#include "WeaponPickup.h"
#include "StarterRifle.h"
#include "KA47Rifle.h"
#include "SMG11Weapon.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

AShooterGameMode::AShooterGameMode()
{
	DefaultPawnClass=AShooterCharacter::StaticClass();
	HUDClass=AShooterHUD::StaticClass();
}
void AShooterGameMode::BeginPlay(){Super::BeginPlay();for(TActorIterator<AWeaponPickup>It(GetWorld());It;++It)return;for(TActorIterator<APlayerStart>It(GetWorld());It;++It){const FVector Base=It->GetActorLocation()+It->GetActorForwardVector()*180.f-FVector(0,0,60.f);const TSubclassOf<AWeaponBase> Classes[3]={AStarterRifle::StaticClass(),AKA47Rifle::StaticClass(),ASMG11Weapon::StaticClass()};for(int32 I=0;I<3;++I){AWeaponPickup* P=GetWorld()->SpawnActor<AWeaponPickup>(AWeaponPickup::StaticClass(),Base+It->GetActorRightVector()*(I-1)*140.f,FRotator(0,90,0));if(P)P->ConfigureWeaponClass(Classes[I]);}break;}}
