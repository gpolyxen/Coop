#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildableStructure.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UNavLinkComponent;

UCLASS(Abstract,Blueprintable)
class MYPROJECT_API ABuildableStructure : public AActor
{
	GENERATED_BODY()
public:
	ABuildableStructure();
	virtual void BeginPlay()override;
	virtual void Tick(float DeltaSeconds)override;
	virtual float TakeDamage(float DamageAmount,const FDamageEvent& DamageEvent,AController* EventInstigator,AActor* DamageCauser)override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	virtual void GetSnapPoints(TArray<FVector>& OutPoints)const;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)USceneComponent* SceneRoot;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Construction")float MaxStructureHealth=350.f;
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly,Category="Construction")float StructureHealth=350.f;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Construction",meta=(ClampMin="1"))int32 AxeHitsToDestroy=10;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Construction")float HalfModuleLength=150.f;
	void SetConstructionPreview(bool bPreview){bConstructionPreview=bPreview;}
	bool IsConstructionPreview()const{return bConstructionPreview;}
	bool IsCollapsing()const{return bCollapsing;}
protected:
	virtual bool HasStructuralSupport()const;
	void BeginCollapse();
	bool bConstructionPreview=false;
	bool bCollapsing=false;
	float SupportCheckDelay=1.f;
	float CollapseVelocityZ=0.f;
	float CollapseDistance=0.f;
	bool bNeedsFoundationSupport=false;
};

UCLASS()
class MYPROJECT_API AWoodWall : public ABuildableStructure
{
	GENERATED_BODY()
public:AWoodWall();
	UPROPERTY(VisibleAnywhere)UInstancedStaticMeshComponent* Pieces;
};

UCLASS()
class MYPROJECT_API AWoodWindowWall : public ABuildableStructure
{
	GENERATED_BODY()
public:AWoodWindowWall();
	UPROPERTY(VisibleAnywhere)UInstancedStaticMeshComponent* Pieces;
};

UCLASS()
class MYPROJECT_API AWoodGate : public ABuildableStructure
{
	GENERATED_BODY()
public:AWoodGate();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	UPROPERTY(VisibleAnywhere)UInstancedStaticMeshComponent* Pieces;
	UPROPERTY(VisibleAnywhere)USceneComponent* LeftHinge;
	UPROPERTY(VisibleAnywhere)USceneComponent* RightHinge;
	UPROPERTY(VisibleAnywhere)UInstancedStaticMeshComponent* DoorPieces;
	UPROPERTY(VisibleAnywhere)UInstancedStaticMeshComponent* RightDoorPieces;
	UFUNCTION(BlueprintCallable)bool TryToggle(AActor* User);
	void SetOpenForLoad(bool bNewOpen);
	UPROPERTY(ReplicatedUsing=OnRep_Open,VisibleAnywhere,BlueprintReadOnly)bool bOpen=false;
private:UFUNCTION()void OnRep_Open();
};

/** A one-leaf human-sized doorway that uses the same interaction, save and AI
 * traversal contract as a gate, but fills an ordinary three-metre wall module. */
UCLASS()
class MYPROJECT_API AWoodDoor : public AWoodGate
{
	GENERATED_BODY()
public:AWoodDoor();
};

UCLASS()
class MYPROJECT_API AWoodFloor : public ABuildableStructure
{
	GENERATED_BODY()
public:AWoodFloor();virtual void GetSnapPoints(TArray<FVector>& OutPoints)const override;
	UPROPERTY(VisibleAnywhere)UInstancedStaticMeshComponent* Pieces;
protected:virtual bool HasStructuralSupport()const override;
};

UCLASS()
class MYPROJECT_API AWoodStairs : public ABuildableStructure
{
	GENERATED_BODY()
public:AWoodStairs();virtual void GetSnapPoints(TArray<FVector>& OutPoints)const override;
	UPROPERTY(VisibleAnywhere)UInstancedStaticMeshComponent* Pieces;
	UPROPERTY(VisibleAnywhere)UNavLinkComponent* NavigationLink;
};

UCLASS()
class MYPROJECT_API AWoodPillar : public ABuildableStructure
{
	GENERATED_BODY()
public:AWoodPillar();
	UPROPERTY(VisibleAnywhere)UStaticMeshComponent* Piece;
};

UCLASS()
class MYPROJECT_API AWallTorch : public ABuildableStructure
{
	GENERATED_BODY()
public:
	AWallTorch();
	virtual void BeginPlay()override;
	virtual void Tick(float DeltaSeconds)override;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Pole;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Bracket;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UStaticMeshComponent* Flame;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)UPointLightComponent* TorchLight;
protected:virtual bool HasStructuralSupport()const override;
private:void UpdateTorchLight();
};
