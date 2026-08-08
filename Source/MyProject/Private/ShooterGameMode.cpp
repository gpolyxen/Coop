#include "ShooterGameMode.h"
#include "ShooterCharacter.h"
#include "ShooterHUD.h"
#include "WeaponPickup.h"
#include "StarterRifle.h"
#include "KA47Rifle.h"
#include "SMG11Weapon.h"
#include "ZombieCharacter.h"
#include "OpenWorldStreamingManager.h"
#include "OpenWorldNavBoundsVolume.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationInvokerComponent.h"

AShooterGameMode::AShooterGameMode()
{
	DefaultPawnClass=AShooterCharacter::StaticClass();
	HUDClass=AShooterHUD::StaticClass();
}
AActor* AShooterGameMode::EnsurePlayerStart()
{
	for(TActorIterator<APlayerStart> It(GetWorld());It;++It)return *It;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(),FVector(0.f,0.f,400.f),FRotator::ZeroRotator,SpawnParameters);
}
AActor* AShooterGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if(AActor* Existing=Super::ChoosePlayerStart_Implementation(Player))return Existing;
	return EnsurePlayerStart();
}
void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();
	APlayerStart* Start=Cast<APlayerStart>(EnsurePlayerStart());
	if(!Start)return;
	bool bHasOpenWorldBounds=false;
	for(TActorIterator<AOpenWorldNavBoundsVolume> It(GetWorld());It;++It){bHasOpenWorldBounds=true;break;}
	if(!bHasOpenWorldBounds)GetWorld()->SpawnActor<AOpenWorldNavBoundsVolume>();
	AOpenWorldStreamingManager* StreamingManager=nullptr;
	for(TActorIterator<AOpenWorldStreamingManager> It(GetWorld());It;++It){StreamingManager=*It;break;}
	if(!StreamingManager)StreamingManager=GetWorld()->SpawnActor<AOpenWorldStreamingManager>();
	if(StreamingManager)StreamingManager->PrepareStartingTile(Start->GetActorLocation()+FVector(GetWorld()->OriginLocation));

	bool bHasWeaponPickup=false;
	for(TActorIterator<AWeaponPickup> It(GetWorld());It;++It){bHasWeaponPickup=true;break;}
	if(!bHasWeaponPickup)
	{
		FVector Base=Start->GetActorLocation()+Start->GetActorForwardVector()*300.f;
		Base.Z=100.f;
		const TSubclassOf<AWeaponBase> Classes[3]={AStarterRifle::StaticClass(),AKA47Rifle::StaticClass(),ASMG11Weapon::StaticClass()};
		for(int32 Index=0;Index<3;++Index)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FVector SpawnLocation=Base+Start->GetActorRightVector()*(Index-1)*180.f;
			AWeaponPickup* Pickup=GetWorld()->SpawnActor<AWeaponPickup>(AWeaponPickup::StaticClass(),SpawnLocation,FRotator(0,90,0),SpawnParameters);
			if(Pickup)
			{
				Pickup->ConfigureWeaponClass(Classes[Index]);
				UE_LOG(LogTemp,Display,TEXT("Spawned initial weapon pickup %d at %s"),Index,*SpawnLocation.ToString());
			}
			else UE_LOG(LogTemp,Error,TEXT("Failed to spawn initial weapon pickup %d"),Index);
		}
	}

	bool bHasZombie=false;
	for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It){bHasZombie=true;break;}
	if(!bHasZombie)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FVector SpawnLocation=Start->GetActorLocation()+Start->GetActorForwardVector()*1800.f;
		SpawnLocation.Z=100.f;
		FVector LookDirection=Start->GetActorLocation()-SpawnLocation;
		LookDirection.Z=0.f;
		const FRotator SpawnRotation=LookDirection.IsNearlyZero()?Start->GetActorRotation():LookDirection.Rotation();
		if(GetWorld()->SpawnActor<AZombieCharacter>(AZombieCharacter::StaticClass(),SpawnLocation,SpawnRotation,SpawnParameters))
		{
			UE_LOG(LogTemp,Display,TEXT("Spawned initial zombie at %s"),*SpawnLocation.ToString());
		}
		else
		{
			UE_LOG(LogTemp,Error,TEXT("Failed to spawn initial zombie"));
		}
	}

	if(UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		for(TActorIterator<ANavMeshBoundsVolume> It(GetWorld());It;++It)Navigation->OnNavigationBoundsUpdated(*It);
		for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)if(It->NavigationInvoker)It->NavigationInvoker->RegisterWithNavigationSystem(*Navigation);
		for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It)if(It->NavigationInvoker)It->NavigationInvoker->RegisterWithNavigationSystem(*Navigation);
		Navigation->Build();
		UE_LOG(LogTemp,Display,TEXT("Requested initial dynamic navigation build"));
	}
}
