#include "HealthPickup.h"

#include "HealthArmorComponent.h"
#include "ShooterCharacter.h"
#include "InventoryComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHealthPickup::AHealthPickup()
{
	PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.12f;
	ItemId=TEXT("Medkit");
	Quantity=1;
	PhysicsBody=CreateDefaultSubobject<UBoxComponent>(TEXT("PickupPhysicsBody"));RootComponent=PhysicsBody;PhysicsBody->SetBoxExtent(FVector(22.f,16.f,9.f));PhysicsBody->SetCollisionProfileName(TEXT("PhysicsActor"));PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);PhysicsBody->SetEnableGravity(true);PhysicsBody->SetUseCCD(true);PhysicsBody->SetSimulatePhysics(true);Mesh->SetupAttachment(PhysicsBody);InteractionRange->SetupAttachment(PhysicsBody);
	InteractionRange->SetSphereRadius(220.f);
	Mesh->SetLinearDamping(1.5f);
	Mesh->SetAngularDamping(3.f);
	Mesh->SetSimulatePhysics(false);Mesh->SetEnableGravity(false);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MedkitMesh(TEXT("/Game/ThirdPersonBP/Items/Small_Medkit/tactical_medkit_pouch.tactical_medkit_pouch"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Chrome(TEXT("/Game/StarterContent/Materials/M_Metal_Chrome.M_Metal_Chrome"));
	if(MedkitMesh.Succeeded())Mesh->SetStaticMesh(MedkitMesh.Object);else if(Cube.Succeeded())Mesh->SetStaticMesh(Cube.Object);
	if(!MedkitMesh.Succeeded()&&Chrome.Succeeded())Mesh->SetMaterial(0,Chrome.Object);
	Mesh->SetRelativeScale3D(MedkitMesh.Succeeded()?FVector(.12f):FVector(.16f,.38f,.14f));

	CrossBar=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HealthCrossBar"));
	CrossBar->SetupAttachment(RootComponent);
	CrossBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if(Cube.Succeeded())CrossBar->SetStaticMesh(Cube.Object);
	if(Chrome.Succeeded())CrossBar->SetMaterial(0,Chrome.Object);
	CrossBar->SetRelativeScale3D(FVector(2.35f,.43f,1.f));
	CrossBar->SetVisibility(!MedkitMesh.Succeeded());

	MarkerLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("HealthMarkerLight"));
	MarkerLight->SetupAttachment(RootComponent);
	MarkerLight->SetRelativeLocation(FVector(0.f,0.f,45.f));
	MarkerLight->SetLightColor(FLinearColor(1.f,.02f,.02f));
	MarkerLight->SetIntensity(0.f);MarkerLight->SetVisibility(false);
	MarkerLight->SetAttenuationRadius(180.f);
	MarkerLight->SetCastShadows(false);

	MarkerText=CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthMarkerText"));
	MarkerText->SetupAttachment(RootComponent);
	MarkerText->SetRelativeLocation(FVector(0.f,0.f,70.f));
	MarkerText->SetRelativeRotation(FRotator(0.f,90.f,0.f));
	MarkerText->SetHorizontalAlignment(EHTA_Center);
	MarkerText->SetWorldSize(30.f);
	MarkerText->SetTextRenderColor(FColor::Red);
	MarkerText->SetText(FText::FromString(TEXT("SMALL MEDKIT")));
	MarkerText->bAlwaysRenderAsText=true;
	MarkerText->SetVisibility(false);

}

void AHealthPickup::BeginPlay(){Super::BeginPlay();if(HasAuthority()&&HasAnyFlags(RF_WasLoaded))Destroy();}
void AHealthPickup::Tick(float DeltaSeconds){Super::Tick(DeltaSeconds);const APlayerController* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;const APawn* Pawn=PC?PC->GetPawn():nullptr;if(MarkerText)MarkerText->SetVisibility(Pawn&&FVector::DistSquared(Pawn->GetActorLocation(),GetActorLocation())<=FMath::Square(280.f));}

bool AHealthPickup::TryPickup(APawn* Pawn)
{
	if(!HasAuthority()||!Pawn||FVector::DistSquared(Pawn->GetActorLocation(),GetActorLocation())>FMath::Square(400.f))return false;
	if(AShooterCharacter* Character=Cast<AShooterCharacter>(Pawn))
	{
		if(Character->Inventory&&Character->Inventory->AddItem(TEXT("Medkit"),FMath::Max(1,Quantity)))
		{
			UE_LOG(LogTemp,Display,TEXT("Player %s picked up a medkit"),*Character->GetName());
			Character->ShowLocalNotification(TEXT("АПТЕЧКА ДОБАВЛЕНА В ИНВЕНТАРЬ"));
			Destroy();
			return true;
		}
	}
	return false;
}
