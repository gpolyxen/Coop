#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildableStructure.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

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
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Construction")float HalfModuleLength=150.f;
	void SetConstructionPreview(bool bPreview){bConstructionPreview=bPreview;}
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
class MYPROJECT_API AWoodGate : public ABuildableStructure
{
	GENERATED_BODY()
public:AWoodGate();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	UPROPERTY(VisibleAnywhere)UInstancedStaticMeshComponent* Pieces;
	UPROPERTY(VisibleAnywhere)UInstancedStaticMeshComponent* DoorPieces;
	UFUNCTION(BlueprintCallable)bool TryToggle(AActor* User);
	UPROPERTY(ReplicatedUsing=OnRep_Open,VisibleAnywhere,BlueprintReadOnly)bool bOpen=false;
private:UFUNCTION()void OnRep_Open();
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
};

UCLASS()
class MYPROJECT_API AWoodPillar : public ABuildableStructure
{
	GENERATED_BODY()
public:AWoodPillar();
	UPROPERTY(VisibleAnywhere)UStaticMeshComponent* Piece;
};
