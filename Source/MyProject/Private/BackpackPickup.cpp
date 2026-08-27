#include "BackpackPickup.h"

#include "InventoryComponent.h"
#include "ShooterCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABackpackPickup::ABackpackPickup()
{
	ItemId=TEXT("Backpack");
	InteractionRange->SetSphereRadius(230.f);
	Mesh->SetLinearDamping(2.f);
	Mesh->SetAngularDamping(4.f);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyMaterial(TEXT("/Game/StarterContent/Materials/M_Wood_Walnut.M_Wood_Walnut"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> StrapMaterial(TEXT("/Game/StarterContent/Materials/M_Metal_Burnished_Steel.M_Metal_Burnished_Steel"));
	if(Cube.Succeeded())Mesh->SetStaticMesh(Cube.Object);
	if(BodyMaterial.Succeeded())Mesh->SetMaterial(0,BodyMaterial.Object);
	Mesh->SetRelativeScale3D(FVector(.32f,.14f,.42f));

	auto CreatePart=[this](const TCHAR* Name,const FVector& Location,const FVector& Scale)
	{
		UStaticMeshComponent* Part=CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Mesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if(Cube.Succeeded())Part->SetStaticMesh(Cube.Object);
		if(StrapMaterial.Succeeded())Part->SetMaterial(0,StrapMaterial.Object);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		return Part;
	};
	TopPocket=CreatePart(TEXT("TopPocket"),FVector(0.f,-48.f,28.f),FVector(.78f,.3f,.22f));
	LeftStrap=CreatePart(TEXT("LeftStrap"),FVector(0.f,72.f,0.f),FVector(.18f,.08f,.9f));
	RightStrap=CreatePart(TEXT("RightStrap"),FVector(0.f,-72.f,0.f),FVector(.18f,.08f,.9f));

	MarkerLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("BackpackMarkerLight"));
	MarkerLight->SetupAttachment(RootComponent);
	MarkerLight->SetRelativeLocation(FVector(0.f,0.f,70.f));
	MarkerLight->SetAttenuationRadius(450.f);
	MarkerText=CreateDefaultSubobject<UTextRenderComponent>(TEXT("BackpackMarkerText"));
	MarkerText->SetupAttachment(RootComponent);
	MarkerText->SetRelativeLocation(FVector(0.f,0.f,85.f));
	MarkerText->SetRelativeRotation(FRotator(0.f,90.f,0.f));
	MarkerText->SetHorizontalAlignment(EHTA_Center);
	MarkerText->SetWorldSize(24.f);
	MarkerText->bAlwaysRenderAsText=true;
	RefreshAppearance();
}

void ABackpackPickup::ConfigureCapacity(int32 InCapacity){Capacity=InCapacity>=20?20:12;RefreshAppearance();}
void ABackpackPickup::OnRep_Capacity(){RefreshAppearance();}
void ABackpackPickup::RefreshAppearance()
{
	const bool bExpedition=Capacity>=20;
	if(MarkerLight){MarkerLight->SetLightColor(bExpedition?FLinearColor(.1f,.5f,1.f):FLinearColor(.15f,1.f,.35f));MarkerLight->SetIntensity(3200.f);}
	if(MarkerText){MarkerText->SetText(FText::FromString(bExpedition?TEXT("EXPEDITION PACK 20"):TEXT("BACKPACK 12")));MarkerText->SetTextRenderColor(bExpedition?FColor(50,160,255):FColor(60,255,100));}
	if(Mesh)Mesh->SetRelativeScale3D(bExpedition?FVector(.4f,.18f,.5f):FVector(.32f,.14f,.42f));
}

bool ABackpackPickup::TryPickup(APawn* Pawn)
{
	if(!HasAuthority()||!Pawn||FVector::DistSquared(Pawn->GetActorLocation(),GetActorLocation())>FMath::Square(420.f))return false;
	if(UInventoryComponent* Inventory=Pawn->FindComponentByClass<UInventoryComponent>())
		if(Inventory->UpgradeCapacity(Capacity))
		{
			if(AShooterCharacter* Character=Cast<AShooterCharacter>(Pawn))Character->ShowLocalNotification(FString::Printf(TEXT("ИНВЕНТАРЬ РАСШИРЕН: %d ЯЧЕЕК"),Capacity),4.f);
			Destroy();
			return true;
		}
	return false;
}

void ABackpackPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABackpackPickup,Capacity);
}
