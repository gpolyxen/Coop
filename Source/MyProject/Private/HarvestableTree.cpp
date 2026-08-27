#include "HarvestableTree.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInterface.h"
#include "ShooterCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "WoodAxeWeapon.h"

AHarvestableTree::AHarvestableTree()
{
	PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.bStartWithTickEnabled=false;bReplicates=true;SetReplicateMovement(false);SetCanBeDamaged(true);
	SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));RootComponent=SceneRoot;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Trunk=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trunk"));Trunk->SetupAttachment(SceneRoot);
	if(Cylinder.Succeeded())Trunk->SetStaticMesh(Cylinder.Object);Trunk->SetRelativeLocation(FVector(0.f,0.f,210.f));Trunk->SetRelativeScale3D(FVector(.55f,.55f,4.2f));Trunk->SetCollisionProfileName(TEXT("BlockAll"));Trunk->SetCanEverAffectNavigation(true);
	Crown=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crown"));Crown->SetupAttachment(SceneRoot);
	if(Sphere.Succeeded())Crown->SetStaticMesh(Sphere.Object);Crown->SetRelativeLocation(FVector(0.f,0.f,520.f));Crown->SetRelativeScale3D(FVector(1.75f,1.75f,2.25f));Crown->SetCollisionEnabled(ECollisionEnabled::NoCollision);Crown->SetCanEverAffectNavigation(false);
	CrownLeft=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrownLeft"));CrownLeft->SetupAttachment(SceneRoot);if(Sphere.Succeeded())CrownLeft->SetStaticMesh(Sphere.Object);CrownLeft->SetRelativeLocation(FVector(-105.f,15.f,455.f));CrownLeft->SetRelativeScale3D(FVector(1.35f,1.45f,1.65f));CrownLeft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrownRight=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrownRight"));CrownRight->SetupAttachment(SceneRoot);if(Sphere.Succeeded())CrownRight->SetStaticMesh(Sphere.Object);CrownRight->SetRelativeLocation(FVector(100.f,-25.f,470.f));CrownRight->SetRelativeScale3D(FVector(1.4f,1.3f,1.7f));CrownRight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ApplyMaterials();
}

void AHarvestableTree::OnConstruction(const FTransform& Transform){Super::OnConstruction(Transform);ApplyMaterials();}
void AHarvestableTree::BeginPlay(){Super::BeginPlay();ApplyMaterials();}
void AHarvestableTree::ApplyMaterials()
{
	UMaterialInterface* Bark=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/StarterContent/Materials/M_Wood_Pine.M_Wood_Pine"));
	UMaterialInterface* Leaves=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/StarterContent/Materials/M_Ground_Moss.M_Ground_Moss"));
	if(Trunk&&Bark)Trunk->SetMaterial(0,Bark);
	for(UStaticMeshComponent* Part:{Crown,CrownLeft,CrownRight})if(Part&&Leaves)Part->SetMaterial(0,Leaves);
}

void AHarvestableTree::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);if(!HasAuthority()||!bFalling)return;
	FallElapsed+=DeltaSeconds;const float CurrentRoll=GetActorRotation().Roll;const float TargetRoll=FallRollSign*88.f;SetActorRotation(FRotator(0.f,GetActorRotation().Yaw,FMath::FInterpConstantTo(CurrentRoll,TargetRoll,DeltaSeconds,62.f)));
	if(FallElapsed>=1.65f){SetActorTickEnabled(false);SetLifeSpan(.55f);}
}

float AHarvestableTree::TakeDamage(float Amount,const FDamageEvent&,AController* EventInstigator,AActor* DamageCauser)
{
	if(!HasAuthority()||Amount<=0.f)return 0.f;
	AWoodAxeWeapon* Axe=Cast<AWoodAxeWeapon>(DamageCauser);if(!Axe)return 0.f;
	AShooterCharacter* Gatherer=Cast<AShooterCharacter>(Axe->GetOwner());if(!Gatherer&&EventInstigator)Gatherer=Cast<AShooterCharacter>(EventInstigator->GetPawn());
	++AxeHits;ForceNetUpdate();
	if(AxeHits<RequiredAxeHits){if(Gatherer)Gatherer->ShowLocalNotification(FString::Printf(TEXT("ДЕРЕВО: ОСТАЛОСЬ УДАРОВ %d"),RequiredAxeHits-AxeHits),1.f);return Amount;}
	if(Gatherer&&Gatherer->Inventory)
	{
		const int32 Added=Gatherer->Inventory->AddItemPartial(TEXT("Wood"),WoodReward);
		if(Added>0)Gatherer->ShowLocalNotification(FString::Printf(TEXT("ДЕРЕВО +%d"),Added),2.f);
		else Gatherer->ShowLocalNotification(TEXT("ИНВЕНТАРЬ ЗАПОЛНЕН"),2.f);
	}
	bFalling=true;SetCanBeDamaged(false);SetReplicateMovement(true);FallRollSign=Gatherer&&FVector::DotProduct(Gatherer->GetActorRightVector(),GetActorLocation()-Gatherer->GetActorLocation())<0.f?-1.f:1.f;SetActorTickEnabled(true);ForceNetUpdate();return Amount;
}

void AHarvestableTree::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AHarvestableTree,AxeHits);DOREPLIFETIME(AHarvestableTree,bFalling);
}
