#include "AmmoPickup.h"

#include "ShooterCharacter.h"
#include "InventoryComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AAmmoPickup::AAmmoPickup()
{
	PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.12f;
	ItemId=TEXT("AK74Ammo");
	Quantity=45;
	PhysicsBody=CreateDefaultSubobject<UBoxComponent>(TEXT("PickupPhysicsBody"));RootComponent=PhysicsBody;PhysicsBody->SetBoxExtent(FVector(20.f,12.f,8.f));PhysicsBody->SetCollisionProfileName(TEXT("PhysicsActor"));PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);PhysicsBody->SetEnableGravity(true);PhysicsBody->SetUseCCD(true);PhysicsBody->SetSimulatePhysics(true);Mesh->SetupAttachment(PhysicsBody);InteractionRange->SetupAttachment(PhysicsBody);
	InteractionRange->SetSphereRadius(220.f);
	Mesh->SetLinearDamping(1.5f);
	Mesh->SetAngularDamping(3.f);
	Mesh->SetSimulatePhysics(false);Mesh->SetEnableGravity(false);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AmmoMesh(TEXT("/Game/ThirdPersonBP/Items/7_62/ammo-ak-47.ammo-ak-47"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FallbackMesh(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	if(AmmoMesh.Succeeded())Mesh->SetStaticMesh(AmmoMesh.Object);
	else if(FallbackMesh.Succeeded())Mesh->SetStaticMesh(FallbackMesh.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AmmoMaterial(TEXT("/Game/StarterContent/Materials/M_Metal_Gold.M_Metal_Gold"));
	if(!AmmoMesh.Succeeded()&&AmmoMaterial.Succeeded())Mesh->SetMaterial(0,AmmoMaterial.Object);
	Mesh->SetRelativeScale3D(AmmoMesh.Succeeded()?FVector(.05f):FVector(.34f,.24f,.16f));

	MarkerLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("AmmoMarkerLight"));
	MarkerLight->SetupAttachment(RootComponent);
	MarkerLight->SetRelativeLocation(FVector(0.f,0.f,45.f));
	MarkerLight->SetLightColor(FLinearColor(1.f,.65f,.05f));
	MarkerLight->SetIntensity(0.f);MarkerLight->SetVisibility(false);
	MarkerLight->SetAttenuationRadius(160.f);
	MarkerLight->SetCastShadows(false);

	MarkerText=CreateDefaultSubobject<UTextRenderComponent>(TEXT("AmmoMarkerText"));
	MarkerText->SetupAttachment(RootComponent);
	MarkerText->SetRelativeLocation(FVector(0.f,0.f,65.f));
	MarkerText->SetRelativeRotation(FRotator(0.f,90.f,0.f));
	MarkerText->SetHorizontalAlignment(EHTA_Center);
	MarkerText->SetWorldSize(28.f);
	MarkerText->SetTextRenderColor(FColor(255,190,20));
	MarkerText->SetText(FText::FromString(TEXT("7.62 MM")));
	MarkerText->bAlwaysRenderAsText=true;
	MarkerText->SetVisibility(false);

}

void AAmmoPickup::BeginPlay(){Super::BeginPlay();if(HasAuthority()&&HasAnyFlags(RF_WasLoaded))Destroy();}
void AAmmoPickup::Tick(float DeltaSeconds){Super::Tick(DeltaSeconds);const APlayerController* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;const APawn* Pawn=PC?PC->GetPawn():nullptr;if(MarkerText)MarkerText->SetVisibility(Pawn&&FVector::DistSquared(Pawn->GetActorLocation(),GetActorLocation())<=FMath::Square(280.f));}

bool AAmmoPickup::TryPickup(APawn* Pawn)
{
	if(!HasAuthority()||!Pawn||FVector::DistSquared(Pawn->GetActorLocation(),GetActorLocation())>FMath::Square(400.f))return false;
	if(AShooterCharacter* Character=Cast<AShooterCharacter>(Pawn))
	{
		if(Character->Inventory&&Character->Inventory->AddItem(AmmoType,FMath::Max(1,Quantity)))
		{
			UE_LOG(LogTemp,Display,TEXT("Player %s added %d %s rounds to inventory"),*Character->GetName(),Quantity,*AmmoType.ToString());
			Character->ShowLocalNotification(TEXT("ПАТРОНЫ АК ДОБАВЛЕНЫ В ИНВЕНТАРЬ"));
			Destroy();
			return true;
		}
	}
	return false;
}
