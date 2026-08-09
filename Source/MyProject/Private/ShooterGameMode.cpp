#include "ShooterGameMode.h"
#include "ShooterCharacter.h"
#include "ShooterHUD.h"
#include "ShooterGameInstance.h"
#include "SaveBed.h"
#include "WeaponPickup.h"
#include "StarterRifle.h"
#include "KA47Rifle.h"
#include "SMG11Weapon.h"
#include "ASValRifle.h"
#include "ZombieCharacter.h"
#include "ZombieSpawnManager.h"
#include "WindField.h"
#include "OpenWorldStreamingManager.h"
#include "OpenWorldNavBoundsVolume.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationInvokerComponent.h"
#include "UObject/ConstructorHelpers.h"

AShooterGameMode::AShooterGameMode()
{
	DefaultPawnClass=AShooterCharacter::StaticClass();
	static ConstructorHelpers::FClassFinder<AShooterCharacter> ConfigurablePlayer(TEXT("/Game/ThirdPersonBP/Player_0/BP_ShooterCharacter"));
	if(ConfigurablePlayer.Succeeded())DefaultPawnClass=ConfigurablePlayer.Class;
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
void AShooterGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	if(!NewPlayer)return;
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())
		if(AShooterCharacter* Character=Cast<AShooterCharacter>(NewPlayer->GetPawn()))GI->ApplyPendingSave(Character);
}
void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();
	APlayerStart* Start=Cast<APlayerStart>(EnsurePlayerStart());
	if(!Start)return;
	if(UShooterGameInstance* GI=GetGameInstance<UShooterGameInstance>())
	{
		FTransform SavedTransform;
		if(GI->GetPendingPlayerTransform(SavedTransform))Start->SetActorLocationAndRotation(SavedTransform.GetLocation(),SavedTransform.Rotator());
	}
	bool bHasOpenWorldBounds=false;
	for(TActorIterator<AOpenWorldNavBoundsVolume> It(GetWorld());It;++It){bHasOpenWorldBounds=true;break;}
	if(!bHasOpenWorldBounds)GetWorld()->SpawnActor<AOpenWorldNavBoundsVolume>();
	AOpenWorldStreamingManager* StreamingManager=nullptr;
	for(TActorIterator<AOpenWorldStreamingManager> It(GetWorld());It;++It){StreamingManager=*It;break;}
	if(!StreamingManager)StreamingManager=GetWorld()->SpawnActor<AOpenWorldStreamingManager>();
	if(StreamingManager)StreamingManager->PrepareStartingTile(Start->GetActorLocation()+FVector(GetWorld()->OriginLocation));
	bool bHasWind=false;for(TActorIterator<AWindField> It(GetWorld());It;++It){bHasWind=true;break;}if(!bHasWind)GetWorld()->SpawnActor<AWindField>();
	bool bHasSpawnManager=false;for(TActorIterator<AZombieSpawnManager> It(GetWorld());It;++It){bHasSpawnManager=true;break;}if(!bHasSpawnManager)GetWorld()->SpawnActor<AZombieSpawnManager>();
	bool bHasSaveBed=false;for(TActorIterator<ASaveBed> It(GetWorld());It;++It){bHasSaveBed=true;break;}
	if(!bHasSaveBed)
	{
		FVector BedLocation=Start->GetActorLocation()-Start->GetActorForwardVector()*350.f+Start->GetActorRightVector()*420.f;
		FHitResult GroundHit;
		if(GetWorld()->LineTraceSingleByChannel(GroundHit,BedLocation+FVector(0.f,0.f,1500.f),BedLocation-FVector(0.f,0.f,2500.f),ECC_WorldStatic))BedLocation.Z=GroundHit.ImpactPoint.Z;
		FActorSpawnParameters Parameters;Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		GetWorld()->SpawnActor<ASaveBed>(ASaveBed::StaticClass(),BedLocation,Start->GetActorRotation(),Parameters);
	}

	bool bHasWeaponPickup=false;
	for(TActorIterator<AWeaponPickup> It(GetWorld());It;++It){bHasWeaponPickup=true;break;}
	if(!bHasWeaponPickup)
	{
		FVector Base=Start->GetActorLocation()+Start->GetActorForwardVector()*300.f;
		Base.Z=100.f;
		const TSubclassOf<AWeaponBase> Classes[4]={AStarterRifle::StaticClass(),AKA47Rifle::StaticClass(),ASMG11Weapon::StaticClass(),AASValRifle::StaticClass()};
		for(int32 Index=0;Index<4;++Index)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FVector SpawnLocation=Base+Start->GetActorRightVector()*(Index-1.5f)*180.f;
			AWeaponPickup* Pickup=GetWorld()->SpawnActor<AWeaponPickup>(AWeaponPickup::StaticClass(),SpawnLocation,FRotator(0,90,0),SpawnParameters);
			if(Pickup)
			{
				Pickup->ConfigureWeaponClass(Classes[Index]);
				UE_LOG(LogTemp,Display,TEXT("Spawned initial weapon pickup %d at %s"),Index,*SpawnLocation.ToString());
			}
			else UE_LOG(LogTemp,Error,TEXT("Failed to spawn initial weapon pickup %d"),Index);
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
