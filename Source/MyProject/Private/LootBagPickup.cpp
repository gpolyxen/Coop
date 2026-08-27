#include "LootBagPickup.h"

#include "InventoryComponent.h"
#include "ShooterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALootBagPickup::ALootBagPickup()
{
	ItemId=TEXT("LootBag");
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BagMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if(BagMesh.Succeeded())Mesh->SetStaticMesh(BagMesh.Object);
	if(BasicMaterial.Succeeded())Mesh->SetMaterial(0,BasicMaterial.Object);
	Mesh->SetRelativeScale3D(FVector(.31f,.25f,.36f));
	Mesh->SetLinearDamping(1.4f);
	Mesh->SetAngularDamping(2.2f);

	BagTop=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BagTop"));
	BagTop->SetupAttachment(Mesh);
	if(BagMesh.Succeeded())BagTop->SetStaticMesh(BagMesh.Object);
	if(BasicMaterial.Succeeded())BagTop->SetMaterial(0,BasicMaterial.Object);
	BagTop->SetRelativeLocation(FVector(0.f,0.f,50.f));
	BagTop->SetRelativeScale3D(FVector(.48f,.58f,.28f));
	BagTop->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RopeTie=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RopeTie"));
	RopeTie->SetupAttachment(Mesh);
	if(Cylinder.Succeeded())RopeTie->SetStaticMesh(Cylinder.Object);
	if(BasicMaterial.Succeeded())RopeTie->SetMaterial(0,BasicMaterial.Object);
	RopeTie->SetRelativeLocation(FVector(0.f,0.f,35.f));
	RopeTie->SetRelativeScale3D(FVector(.54f,.65f,.10f));
	RopeTie->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StickA=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StickA"));
	StickA->SetupAttachment(Mesh);
	if(Cylinder.Succeeded())StickA->SetStaticMesh(Cylinder.Object);
	if(BasicMaterial.Succeeded())StickA->SetMaterial(0,BasicMaterial.Object);
	StickA->SetRelativeLocation(FVector(0.f,-25.f,20.f));
	StickA->SetRelativeRotation(FRotator(0.f,35.f,78.f));
	StickA->SetRelativeScale3D(FVector(.10f,.10f,.85f));
	StickA->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StickB=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StickB"));
	StickB->SetupAttachment(Mesh);
	if(Cylinder.Succeeded())StickB->SetStaticMesh(Cylinder.Object);
	if(BasicMaterial.Succeeded())StickB->SetMaterial(0,BasicMaterial.Object);
	StickB->SetRelativeLocation(FVector(0.f,24.f,18.f));
	StickB->SetRelativeRotation(FRotator(0.f,-28.f,82.f));
	StickB->SetRelativeScale3D(FVector(.09f,.09f,.78f));
	StickB->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupText=CreateDefaultSubobject<UTextRenderComponent>(TEXT("PickupText"));
	PickupText->SetupAttachment(Mesh);
	PickupText->SetRelativeLocation(FVector(0.f,0.f,95.f));
	PickupText->SetHorizontalAlignment(EHTA_Center);
	PickupText->SetWorldSize(18.f);
	PickupText->SetTextRenderColor(FColor(255,220,90));
	PickupText->SetText(FText::FromString(TEXT("E  ПРИПАСЫ")));
	PickupText->bAlwaysRenderAsText=true;
}

void ALootBagPickup::BeginPlay()
{
	Super::BeginPlay();
	if(HasAuthority())
	{
		// Wood is now an axe-harvested resource. Zombie bags never create it.
		RemainingWood=0;
		RemainingRope=FMath::RandRange(MinRope,MaxRope);
		RemainingMedicine=FMath::FRand()<=MedicineChance?1:0;
		RemainingBandages=FMath::FRand()<=BandageChance?FMath::RandRange(1,FMath::Max(1,MaxBandages)):0;
		RemainingLeather=FMath::FRand()<=LeatherChance?FMath::RandRange(1,FMath::Max(1,MaxLeather)):0;
		RemainingCloth=FMath::FRand()<=ClothChance?FMath::RandRange(1,FMath::Max(1,MaxCloth)):0;
		RemainingGasoline=FMath::FRand()<=GasolineChance?1:0;
	}
	auto Tint=[this](UStaticMeshComponent* Component,const FLinearColor& Color)
	{
		if(!Component)return;
		if(UMaterialInstanceDynamic* Material=Component->CreateAndSetMaterialInstanceDynamic(0))Material->SetVectorParameterValue(TEXT("Color"),Color);
	};
	Tint(Mesh,FLinearColor(.20f,.055f,.018f));
	Tint(BagTop,FLinearColor(.28f,.08f,.025f));
	Tint(RopeTie,FLinearColor(.72f,.48f,.19f));
	Tint(StickA,FLinearColor(.13f,.035f,.01f));
	Tint(StickB,FLinearColor(.16f,.045f,.012f));
	StickA->SetVisibility(false,true);
	StickB->SetVisibility(false,true);
}

bool ALootBagPickup::TryPickup(APawn* Pawn)
{
	if(!HasAuthority()||!Pawn)return false;
	const float PickupDistance=InteractionRange?InteractionRange->GetScaledSphereRadius()+110.f:250.f;
	if(FVector::DistSquared(Pawn->GetActorLocation(),GetActorLocation())>FMath::Square(PickupDistance))return false;
	UInventoryComponent* Inventory=Pawn->FindComponentByClass<UInventoryComponent>();
	if(!Inventory)return false;
	auto Take=[Inventory](FName Id,int32& Remaining){const int32 Added=Inventory->AddItemPartial(Id,Remaining);Remaining-=Added;return Added;};
	const int32 AddedWood=0,AddedRope=Take(TEXT("Rope"),RemainingRope);
	const int32 AddedMedicine=Take(TEXT("Medicine"),RemainingMedicine),AddedBandages=Take(TEXT("Bandage"),RemainingBandages);
	const int32 AddedLeather=Take(TEXT("Leather"),RemainingLeather),AddedCloth=Take(TEXT("Cloth"),RemainingCloth),AddedGasoline=Take(TEXT("Gasoline"),RemainingGasoline);
	const int32 TotalAdded=AddedWood+AddedRope+AddedMedicine+AddedBandages+AddedLeather+AddedCloth+AddedGasoline;
	if(AShooterCharacter* Shooter=Cast<AShooterCharacter>(Pawn))
	{
		FString Message=TotalAdded>0?FString::Printf(TEXT("ВЗЯТО: ВЕРЁВКИ +%d"),AddedRope):TEXT("ИНВЕНТАРЬ ЗАПОЛНЕН — СОДЕРЖИМОЕ ОСТАЛОСЬ В МЕШКЕ");
		if(AddedMedicine)Message+=TEXT(", МЕДИКАМЕНТ +1");if(AddedBandages)Message+=FString::Printf(TEXT(", БИНТЫ +%d"),AddedBandages);if(AddedLeather)Message+=FString::Printf(TEXT(", КОЖА +%d"),AddedLeather);if(AddedCloth)Message+=FString::Printf(TEXT(", ТКАНЬ +%d"),AddedCloth);if(AddedGasoline)Message+=TEXT(", БЕНЗИН +1");
		Shooter->ShowLocalNotification(Message);
	}
	const bool bEmpty=RemainingWood+RemainingRope+RemainingMedicine+RemainingBandages+RemainingLeather+RemainingCloth+RemainingGasoline<=0;
	if(bEmpty)Destroy();else ForceNetUpdate();
	return TotalAdded>0;
}

void ALootBagPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALootBagPickup,RemainingWood);DOREPLIFETIME(ALootBagPickup,RemainingRope);DOREPLIFETIME(ALootBagPickup,RemainingMedicine);
	DOREPLIFETIME(ALootBagPickup,RemainingBandages);DOREPLIFETIME(ALootBagPickup,RemainingLeather);DOREPLIFETIME(ALootBagPickup,RemainingCloth);DOREPLIFETIME(ALootBagPickup,RemainingGasoline);
}
