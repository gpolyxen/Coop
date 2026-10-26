#include "HarvestableTree.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInterface.h"
#include "ShooterCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "WoodAxeWeapon.h"

AHarvestableTree::AHarvestableTree()
{
	PrimaryActorTick.bCanEverTick = true;PrimaryActorTick.bStartWithTickEnabled = false;bReplicates = true;SetReplicateMovement(false);SetCanBeDamaged(true);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));RootComponent = SceneRoot;
	static ConstructorHelpers::FObjectFinder<UStaticMesh>TreeAsset(TEXT("/Game/ThirdPersonBP/Items/Three_00/three_00.three_00"));
	Trunk = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TreeAsset"));Trunk->SetupAttachment(SceneRoot);
	if (TreeAsset.Succeeded())
	{
		Trunk->SetStaticMesh(TreeAsset.Object);
	}
	Trunk->SetRelativeLocation(FVector::ZeroVector);Trunk->SetRelativeRotation(FRotator::ZeroRotator);Trunk->SetRelativeScale3D(FVector(1.f));Trunk->SetCollisionEnabled(ECollisionEnabled::NoCollision);Trunk->SetCanEverAffectNavigation(false);
	TrunkCollision=CreateDefaultSubobject<UCapsuleComponent>(TEXT("TrunkCollision"));TrunkCollision->SetupAttachment(SceneRoot);TrunkCollision->SetRelativeLocation(FVector(0.f,0.f,230.f));TrunkCollision->SetCapsuleRadius(58.f);TrunkCollision->SetCapsuleHalfHeight(230.f);TrunkCollision->SetCollisionProfileName(TEXT("BlockAll"));TrunkCollision->SetCanEverAffectNavigation(true);
	Crown=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crown"));Crown->SetupAttachment(SceneRoot);
	CrownLeft=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrownLeft"));CrownLeft->SetupAttachment(SceneRoot);
	CrownRight=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrownRight"));CrownRight->SetupAttachment(SceneRoot);
	for(UStaticMeshComponent* OldPart:{Crown,CrownLeft,CrownRight}){OldPart->SetVisibility(false,true);OldPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);OldPart->SetCanEverAffectNavigation(false);}
	ApplyMaterials();
}

void AHarvestableTree::OnConstruction(const FTransform& Transform){Super::OnConstruction(Transform);ApplyTreeVariant();ApplyMaterials();}
void AHarvestableTree::BeginPlay(){Super::BeginPlay();ApplyTreeVariant();ApplyMaterials();}
void AHarvestableTree::ApplyTreeVariant()
{
	if(!Trunk)return;
	static UStaticMesh* TreeMeshes[]={
		LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ThirdPersonBP/Items/Three_00/three_00.three_00")),
		LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ThirdPersonBP/Items/Three_01/Three_01.Three_01")),
		LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ThirdPersonBP/Items/Three_02/Three_02.Three_02"))};
	const FVector Position=GetActorLocation();
	const uint32 LocationHash=HashCombine(GetTypeHash(FMath::RoundToInt(Position.X)),GetTypeHash(FMath::RoundToInt(Position.Y)));
	UStaticMesh* Selected=TreeMeshes[LocationHash%UE_ARRAY_COUNT(TreeMeshes)];
	if(!Selected)return;
	Trunk->SetStaticMesh(Selected);
	const FBoxSphereBounds Bounds=Selected->GetBounds();
	const float SourceHeight=FMath::Max(Bounds.BoxExtent.Z*2.f,1.f);
	const float VisualScale=750.f/SourceHeight;
	const float BottomZ=Bounds.Origin.Z-Bounds.BoxExtent.Z;
	Trunk->SetRelativeScale3D(FVector(VisualScale));
	Trunk->SetRelativeLocation(FVector(-Bounds.Origin.X*VisualScale,-Bounds.Origin.Y*VisualScale,-BottomZ*VisualScale));
}
void AHarvestableTree::ApplyMaterials()
{
	// The imported tree already owns its bark and leaf material slots.
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
