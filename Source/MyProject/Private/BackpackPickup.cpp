#include "BackpackPickup.h"

#include "InventoryComponent.h"
#include "ShooterCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABackpackPickup::ABackpackPickup()
{
	ItemId=TEXT("Backpack");
	PhysicsBox=CreateDefaultSubobject<UBoxComponent>(TEXT("BackpackPhysicsBox"));
	PhysicsBox->SetBoxExtent(FVector(28.f,18.f,37.5f));
	PhysicsBox->SetCollisionProfileName(TEXT("PhysicsActor"));
	PhysicsBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsBox->SetSimulatePhysics(true);
	PhysicsBox->SetEnableGravity(true);
	PhysicsBox->SetLinearDamping(2.f);
	PhysicsBox->SetAngularDamping(4.f);
	RootComponent=PhysicsBox;
	Mesh->SetupAttachment(PhysicsBox);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetEnableGravity(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionRange->SetSphereRadius(230.f);
	Mesh->SetLinearDamping(2.f);
	Mesh->SetAngularDamping(4.f);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SmallBag(TEXT("/Game/ThirdPersonBP/Items/Small_Bag/small_bag.small_bag"));
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
	if(SmallBag.Succeeded())
	{
		Mesh->SetStaticMesh(SmallBag.Object);
		// The authored mesh already contains its pockets and straps.
		TopPocket->SetVisibility(false,true);
		LeftStrap->SetVisibility(false,true);
		RightStrap->SetVisibility(false,true);
	}

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

void ABackpackPickup::ConfigureCapacity(int32 InCapacity)
{
	Capacity=InCapacity<=20?20:(InCapacity<=38?38:64);
	RefreshAppearance();
}
void ABackpackPickup::OnRep_Capacity(){RefreshAppearance();}
void ABackpackPickup::RefreshAppearance()
{
	const bool bLarge=Capacity>=64;
	const bool bMedium=Capacity>=38&&!bLarge;
	static UStaticMesh* SmallMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ThirdPersonBP/Items/Small_Bag/small_bag.small_bag"));
	static UStaticMesh* MediumMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ThirdPersonBP/Items/Middle_Bag/medium_bag.medium_bag"));
	static UStaticMesh* LargeMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ThirdPersonBP/Items/Big_Bag/Large_bag.Large_bag"));
	UStaticMesh* SelectedMesh=bLarge?LargeMesh:(bMedium?MediumMesh:SmallMesh);
	const FLinearColor Color=bLarge?FLinearColor(.65f,.2f,1.f):(bMedium?FLinearColor(.1f,.5f,1.f):FLinearColor(.15f,1.f,.35f));
	if(MarkerLight){MarkerLight->SetLightColor(Color);MarkerLight->SetIntensity(3200.f);}
	if(MarkerText)
	{
		const FString Name=bLarge?TEXT("БОЛЬШОЙ РЮКЗАК — 64"):(bMedium?TEXT("СРЕДНИЙ РЮКЗАК — 38"):TEXT("МАЛЫЙ РЮКЗАК — 20"));
		MarkerText->SetText(FText::FromString(Name));MarkerText->SetTextRenderColor(Color.ToFColor(true));
	}
	if(Mesh&&SelectedMesh)
	{
		Mesh->SetStaticMesh(SelectedMesh);
		TopPocket->SetVisibility(false,true);LeftStrap->SetVisibility(false,true);RightStrap->SetVisibility(false,true);
		const FBox Bounds=SelectedMesh->GetBoundingBox();
		const float DesiredHeight=bLarge?105.f:(bMedium?90.f:75.f);
		const float UniformScale=FMath::Clamp(DesiredHeight/FMath::Max(1.f,Bounds.GetSize().Z),.001f,10.f);
		const FVector ScaledSize=Bounds.GetSize()*UniformScale;
		const FVector HalfExtent(FMath::Clamp(ScaledSize.X*.5f,12.f,55.f),FMath::Clamp(ScaledSize.Y*.5f,12.f,55.f),DesiredHeight*.5f);
		Mesh->SetRelativeScale3D(FVector(UniformScale));
		Mesh->SetRelativeRotation(FRotator::ZeroRotator);
		Mesh->SetRelativeLocation(FVector(-Bounds.GetCenter().X*UniformScale,-Bounds.GetCenter().Y*UniformScale,-Bounds.Min.Z*UniformScale));
		if(PhysicsBox)PhysicsBox->SetBoxExtent(HalfExtent);
	}
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
