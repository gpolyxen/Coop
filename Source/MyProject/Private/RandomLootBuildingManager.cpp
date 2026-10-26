#include "RandomLootBuildingManager.h"

#include "BuildableStructure.h"
#include "LootSuitcasePickup.h"
#include "HealthPickup.h"
#include "AmmoPickup.h"
#include "ZombieCharacter.h"
#include "WalkerZombieCharacter.h"
#include "RunnerZombieCharacter.h"
#include "BruteZombieCharacter.h"
#include "ZombieAIController.h"
#include "OpenWorldTile.h"
#include "HarvestableTree.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"

ARandomLootBuildingManager::ARandomLootBuildingManager()
{
	bReplicates=false;PrimaryActorTick.bCanEverTick=false;
}

void ARandomLootBuildingManager::BeginPlay()
{
	Super::BeginPlay();
	if(HasAuthority())GetWorldTimerManager().SetTimer(GenerationTimer,this,&ARandomLootBuildingManager::Generate,1.5f,false);
}

bool ARandomLootBuildingManager::FindGround(const FVector& Candidate,FVector& OutGround)const
{
	// Reserve the whole river corridor and the guaranteed starting lake. The
	// margin includes a four-module building footprint, not just its centre.
	const float RiverCenterY=2800.f+FMath::Sin(Candidate.X/5200.f)*520.f;
	if(FMath::Abs(Candidate.Y-RiverCenterY)<2200.f)return false;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RandomBuildingGround),false,this);float MinimumZ=MAX_flt,MaximumZ=-MAX_flt,SumZ=0.f;
	// Sample the corners of the new 7-8 by 6-7 module footprint. The former
	// 600 cm samples only validated the centre of a large house and allowed its
	// outer rooms to hang over a slope.
	const FVector2D Samples[]={{0,0},{-1200,-1050},{-1200,1050},{1200,-1050},{1200,1050}};
	for(const FVector2D& Offset:Samples){FHitResult Hit;const FVector Point=Candidate+FVector(Offset.X,Offset.Y,0.f);if(!GetWorld()->LineTraceSingleByChannel(Hit,Point+FVector(0,0,3500.f),Point-FVector(0,0,5000.f),ECC_WorldStatic,Query)||Hit.ImpactNormal.Z<.9f)return false;MinimumZ=FMath::Min(MinimumZ,Hit.ImpactPoint.Z);MaximumZ=FMath::Max(MaximumZ,Hit.ImpactPoint.Z);SumZ+=Hit.ImpactPoint.Z;}
	if(MaximumZ-MinimumZ>55.f)return false;OutGround=FVector(Candidate.X,Candidate.Y,SumZ/UE_ARRAY_COUNT(Samples)+3.f);return true;
}

void ARandomLootBuildingManager::Generate()
{
	if(!HasAuthority())return;
	FRandomStream Random(FDateTime::Now().GetMillisecond()+FMath::Rand());
	PendingBuildings.Reset();NextBuildingToSpawn=0;TArray<FVector> Centers;
	for(int32 Attempt=0;Attempt<BuildingCount*24&&PendingBuildings.Num()<BuildingCount;++Attempt)
	{
		const float Angle=Random.FRandRange(0.f,2.f*PI),Distance=Random.FRandRange(MinimumGenerationRadius,MaximumGenerationRadius);
		FVector Ground;if(!FindGround(GenerationCenter+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0.f)*Distance,Ground))continue;
		// The enlarged houses need room for their full footprint as well as a path
		// between neighbouring buildings.
		bool bTooClose=false;const float RequiredSpacing=FMath::Max(MinimumBuildingSpacing,3000.f);
		for(const FVector& Center:Centers)if(FVector::DistSquared2D(Center,Ground)<FMath::Square(RequiredSpacing)){bTooClose=true;break;}
		if(bTooClose)continue;
		const int32 Floors=Random.RandRange(1,FMath::Max(1,MaximumFloors));
		// Generated houses need enough horizontal space for an actual stairwell and
		// rooms.  The old two-module footprint made every upper floor one narrow
		// corridor and left no valid place for another flight of stairs.
		const int32 Width=Random.RandRange(7,8);
		const int32 Depth=Random.RandRange(6,7);
		Centers.Add(Ground);
		FPendingBuilding Plan;Plan.Ground=Ground;Plan.Yaw=FMath::RoundToFloat(Random.FRandRange(0.f,3.f))*90.f;Plan.BuildingIndex=PendingBuildings.Num();Plan.Floors=Floors;Plan.Width=Width;Plan.Depth=Depth;PendingBuildings.Add(Plan);
	}
	SpawnNextBuilding();
}

void ARandomLootBuildingManager::SpawnNextBuilding()
{
	if(!HasAuthority()||!PendingBuildings.IsValidIndex(NextBuildingToSpawn))
	{
		UE_LOG(LogTemp,Display,TEXT("Generated %d random destructible loot buildings"),NextBuildingToSpawn);
		PendingBuildings.Reset();
		return;
	}
	const FPendingBuilding& Plan=PendingBuildings[NextBuildingToSpawn++];
	SpawnBuilding(Plan.Ground,Plan.Yaw,Plan.BuildingIndex,Plan.Floors,Plan.Width,Plan.Depth);
	// Hundreds of destructible modules are intentionally retained, but creating
	// all houses plus their collision and replication state in one frame caused
	// the visible freezes. One house per interval amortizes that work.
	GetWorldTimerManager().SetTimer(GenerationTimer,this,&ARandomLootBuildingManager::SpawnNextBuilding,.35f,false);
}

void ARandomLootBuildingManager::SpawnBuilding(const FVector& GroundLocation,float Yaw,int32 BuildingIndex,int32 FloorCount,int32 WidthModules,int32 DepthModules)
{
	const FRotator BuildingRotation(0.f,Yaw,0.f);const FTransform Basis(BuildingRotation,GroundLocation);
	FActorSpawnParameters Parameters;Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto WorldPoint=[&](float X,float Y,float Z){return Basis.TransformPosition(FVector(X,Y,Z));};
	auto SpawnStructure=[&](UClass* Class,float X,float Y,float Z,float LocalYaw)
	{
		ABuildableStructure* Structure=GetWorld()->SpawnActor<ABuildableStructure>(Class,WorldPoint(X,Y,Z),FRotator(0.f,Yaw+LocalYaw,0.f),Parameters);if(Structure){Structure->Tags.AddUnique(TEXT("GeneratedBuilding"));Structure->SetActorTickEnabled(false);}return Structure;
	};
	auto SpawnDoorPair=[&](float X,float Y,float Z,float LocalYaw)
	{
		const FRotator DoorRotation(0.f,Yaw+LocalYaw,0.f);
		const FVector Along=DoorRotation.RotateVector(FVector::RightVector).GetSafeNormal2D();
		const FVector Centre=WorldPoint(X,Y,Z);
		if(AWoodDoor* Door=GetWorld()->SpawnActor<AWoodDoor>(AWoodDoor::StaticClass(),Centre-Along*75.f,DoorRotation,Parameters)){Door->SetOpenDirection(90.f);Door->Tags.AddUnique(TEXT("GeneratedBuilding"));Door->SetActorTickEnabled(false);}
		FRotator Mirrored=DoorRotation;Mirrored.Yaw+=180.f;
		if(AWoodDoor* Door=GetWorld()->SpawnActor<AWoodDoor>(AWoodDoor::StaticClass(),Centre+Along*75.f,Mirrored,Parameters)){Door->SetOpenDirection(-90.f);Door->Tags.AddUnique(TEXT("GeneratedBuilding"));Door->SetActorTickEnabled(false);}
	};
	FloorCount=FMath::Max(1,FloorCount);WidthModules=FMath::Clamp(WidthModules,7,8);DepthModules=FMath::Clamp(DepthModules,6,7);
	TArray<float> XPositions;for(int32 XIndex=0;XIndex<WidthModules;++XIndex)XPositions.Add((XIndex-(WidthModules-1)*.5f)*300.f);
	TArray<float> YPositions;for(int32 YIndex=0;YIndex<DepthModules;++YIndex)YPositions.Add((YIndex-(DepthModules-1)*.5f)*300.f);
	const float EdgeX=WidthModules*150.f,EdgeY=DepthModules*150.f;
	// Decorations are generated by streaming tiles before houses. Remove every
	// visible rock and its hidden collision proxy from the complete house volume,
	// otherwise a valid terrain decoration can later end up inside a room.
	const FVector LocalExtent(EdgeX+180.f,EdgeY+180.f,FloorCount*150.f+250.f);
	const FVector WorldExtent=BuildingRotation.RotateVector(LocalExtent).GetAbs();
	const FBox HouseBox(GroundLocation+FVector(0.f,0.f,FloorCount*150.f)-WorldExtent,GroundLocation+FVector(0.f,0.f,FloorCount*150.f)+WorldExtent);
	for(TActorIterator<AOpenWorldTile> It(GetWorld());It;++It)for(UHierarchicalInstancedStaticMeshComponent* Group:{It->Rocks,It->RockCollision})if(Group)
	{
		TArray<int32> Instances=Group->GetInstancesOverlappingBox(HouseBox,true);Instances.Sort(TGreater<int32>());for(int32 Instance:Instances)Group->RemoveInstance(Instance);
	}
	const FBox TreeExclusionBox=HouseBox.ExpandBy(350.f);
	for(TActorIterator<AHarvestableTree> It(GetWorld());It;++It)
	{
		AHarvestableTree* Tree=*It;
		if(IsValid(Tree)&&TreeExclusionBox.IsInsideOrOn(Tree->GetActorLocation()))Tree->Destroy();
	}
	const bool bRoofAccess=FloorCount>1&&(BuildingIndex%2==0);
	// Each transition chooses its own interior floor cell. A 3x3 clearance zone
	// around it is later kept free of doors and partitions. Consecutive flights
	// must also use different, non-overlapping zones, so they cannot form the
	// unnatural vertical stack seen in the old two-row houses.
	TArray<FIntPoint> StairCells;TArray<float> StairYaws;
	FRandomStream StairRandom(7919+BuildingIndex*101+FloorCount*17+WidthModules);
	for(int32 Level=0;Level<FloorCount;++Level)
	{
		struct FStairCandidate{FIntPoint Cell;int32 YawStep;};TArray<FStairCandidate> Candidates;
		const FIntPoint BottomDirections[4]={FIntPoint(0,-1),FIntPoint(1,0),FIntPoint(0,1),FIntPoint(-1,0)};
		for(int32 XIndex=1;XIndex<WidthModules-1;++XIndex)for(int32 YIndex=1;YIndex<DepthModules-1;++YIndex)for(int32 YawStep=0;YawStep<4;++YawStep)
		{
			const FIntPoint Candidate(XIndex,YIndex);
			bool bClear=true;const FIntPoint Direction=BottomDirections[YawStep];
			for(int32 Distance=1;Distance<=3;++Distance){const FIntPoint Approach=Candidate+Direction*Distance;if(Approach.X<1||Approach.X>=WidthModules-1||Approach.Y<1||Approach.Y>=DepthModules-1){bClear=false;break;}}
			for(const FIntPoint& Existing:StairCells)
				if(FMath::Abs(Candidate.X-Existing.X)<=2&&FMath::Abs(Candidate.Y-Existing.Y)<=2){bClear=false;break;}
			if(bClear)Candidates.Add({Candidate,YawStep});
		}
		// A very tall 7x6 building may exhaust fully separated cells. Reuse the
		// candidate farthest from the previous opening, never the same cell.
		if(Candidates.Num()==0)
		{
			int32 BestDistance=-1;FIntPoint Best(1,2);const FIntPoint Previous=StairCells.Num()?StairCells.Last():FIntPoint(-10,-10);
			for(int32 XIndex=1;XIndex<WidthModules-1;++XIndex)for(int32 YIndex=2;YIndex<DepthModules-1;++YIndex)
			{
				const int32 Distance=FMath::Abs(XIndex-Previous.X)+FMath::Abs(YIndex-Previous.Y);
				if(Distance>BestDistance){BestDistance=Distance;Best=FIntPoint(XIndex,YIndex);}
			}
			const int32 FallbackYaw=Best.Y>=3?0:2;Candidates.Add({Best,FallbackYaw});
		}
		const FStairCandidate Chosen=Candidates[StairRandom.RandRange(0,Candidates.Num()-1)];StairCells.Add(Chosen.Cell);
		StairYaws.Add(Chosen.YawStep*90.f);
	}
	auto IsNearStair=[&](int32 Level,int32 XIndex,int32 YIndex,int32 Radius)
	{
		for(int32 StairLevel=FMath::Max(0,Level-1);StairLevel<=FMath::Min(Level,StairCells.Num()-1);++StairLevel)
			if(FMath::Abs(XIndex-StairCells[StairLevel].X)<=Radius&&FMath::Abs(YIndex-StairCells[StairLevel].Y)<=Radius)return true;
		return false;
	};
	// Each level is made from ordinary saved/destructible build pieces. Upper
	// levels leave a deliberate stairwell tile so both players and AI have a route.
	for(int32 Level=0;Level<FloorCount;++Level)
	{
		const float Z=Level*300.f;
		for(int32 XIndex=0;XIndex<XPositions.Num();++XIndex)for(int32 YIndex=0;YIndex<YPositions.Num();++YIndex)
		{
			// The outgoing staircase keeps its floor tile underneath it. Only the
			// matching tile on the storey above becomes the stair opening.
			const bool bAboveStairwell=Level>0&&XIndex==StairCells[Level-1].X&&YIndex==StairCells[Level-1].Y;
			if(!bAboveStairwell)SpawnStructure(AWoodFloor::StaticClass(),XPositions[XIndex],YPositions[YIndex],Z+2.f,0.f);
		}
		for(float Y:YPositions)
		{
			SpawnStructure((Level+BuildingIndex)%3==0?AWoodWindowWall::StaticClass():AWoodWall::StaticClass(),-EdgeX,Y,Z,0.f);
			SpawnStructure((Level+BuildingIndex)%2==0?AWoodWindowWall::StaticClass():AWoodWall::StaticClass(),EdgeX,Y,Z,0.f);
		}
		for(int32 XIndex=0;XIndex<XPositions.Num();++XIndex)
		{
			const float X=XPositions[XIndex];
			SpawnStructure((XIndex+Level)%2?AWoodWindowWall::StaticClass():AWoodWall::StaticClass(),X,EdgeY,Z,90.f);
			// One ordinary human-sized entrance. Doors inherit the gate interaction
			// contract, so generated and player-built doors save, replicate and can be
			// opened by the same E interaction without looking like double gates.
			if(Level>0||XIndex!=WidthModules/2)SpawnStructure((XIndex+Level+1)%3==0?AWoodWindowWall::StaticClass():AWoodWall::StaticClass(),X,-EdgeY,Z,90.f);
			else SpawnDoorPair(X,-EdgeY,Z,90.f);
		}

		// Divide every floor into two rooms. The doorway swaps sides on alternating
		// levels so residents can wait behind a real door without blocking the stair
		// landing. This also makes the layouts less like empty boxes.
		const int32 PartitionAfter=WidthModules/2-1;
		const float PartitionX=(XPositions[PartitionAfter]+XPositions[PartitionAfter+1])*.5f;
		const int32 DoorRowA=FMath::Clamp(DepthModules/4,1,DepthModules-2);const int32 DoorRowB=FMath::Clamp((DepthModules*3)/4,1,DepthModules-2);
		for(int32 YIndex=0;YIndex<DepthModules;++YIndex)
		{
			const float Y=YPositions[YIndex];
			if(YIndex==DoorRowA||YIndex==DoorRowB)SpawnDoorPair(PartitionX,Y,Z,0.f);
			else SpawnStructure(AWoodWall::StaticClass(),PartitionX,Y,Z,0.f);
		}
		// A second cross-wall closes the two halves into four real rooms instead of
		// leaving one long open hall on either side of the first partition.
		const int32 CrossAfter=DepthModules/2-1;
		const float PartitionY=(YPositions[CrossAfter]+YPositions[CrossAfter+1])*.5f;
		const int32 DoorColumnA=FMath::Clamp(WidthModules/4,1,WidthModules-2);const int32 DoorColumnB=FMath::Clamp((WidthModules*3)/4,1,WidthModules-2);
		for(int32 XIndex=0;XIndex<WidthModules;++XIndex)
		{
			if(XIndex==DoorColumnA||XIndex==DoorColumnB)SpawnDoorPair(XPositions[XIndex],PartitionY,Z,90.f);
			else SpawnStructure(AWoodWall::StaticClass(),XPositions[XIndex],PartitionY,Z,90.f);
		}

		// A flight exists on every transition, including an optional final flight to
		// the roof. Alternating its direction keeps both landings inside the shared
		// open shaft and prevents the second flight from terminating in a wall.
		if(Level<FloorCount-1||(Level==FloorCount-1&&bRoofAccess))
		{
			SpawnStructure(AWoodStairs::StaticClass(),XPositions[StairCells[Level].X],YPositions[StairCells[Level].Y],Z,StairYaws[Level]);
		}
	}
	for(int32 XIndex=0;XIndex<XPositions.Num();++XIndex)for(int32 YIndex=0;YIndex<YPositions.Num();++YIndex)
	{
		const bool bRoofStairwell=bRoofAccess&&XIndex==StairCells[FloorCount-1].X&&YIndex==StairCells[FloorCount-1].Y;
		if(!bRoofStairwell)SpawnStructure(AWoodFloor::StaticClass(),XPositions[XIndex],YPositions[YIndex],FloorCount*300.f+2.f,0.f);
	}
	// Consumables are generated only as part of a building. Counts are per house,
	// never global: at most three medkits and eight AK ammunition boxes.
	auto SpawnIndoorPickup=[&](UClass* PickupClass,int32 Count)
	{
		for(int32 Index=0;Index<Count;++Index)
		{
			const int32 Level=FMath::RandRange(0,FloorCount-1);
			const int32 XIndex=FMath::RandRange(1,FMath::Max(1,WidthModules-2));
			const int32 YIndex=FMath::RandRange(1,FMath::Max(1,DepthModules-2));
			// Small jitter prevents several pickups from appearing as one object while
			// keeping them well inside the surrounding walls.
			const float JitterX=FMath::FRandRange(-65.f,65.f),JitterY=FMath::FRandRange(-65.f,65.f);
			if(APickupActor* Pickup=GetWorld()->SpawnActor<APickupActor>(PickupClass,WorldPoint(XPositions[XIndex]+JitterX,YPositions[YIndex]+JitterY,Level*300.f+55.f),BuildingRotation,Parameters))Pickup->Tags.AddUnique(TEXT("BuildingLoot"));
		}
	};
	SpawnIndoorPickup(AHealthPickup::StaticClass(),FMath::RandRange(1,3));
	const int32 AmmoCount=FMath::RandRange(1,8);
	for(int32 Index=0;Index<AmmoCount;++Index)
	{
		const int32 Level=FMath::RandRange(0,FloorCount-1),XIndex=FMath::RandRange(1,FMath::Max(1,WidthModules-2)),YIndex=FMath::RandRange(1,FMath::Max(1,DepthModules-2));
		if(AAmmoPickup* Pickup=GetWorld()->SpawnActor<AAmmoPickup>(AAmmoPickup::StaticClass(),WorldPoint(XPositions[XIndex]+FMath::FRandRange(-65.f,65.f),YPositions[YIndex]+FMath::FRandRange(-65.f,65.f),Level*300.f+55.f),BuildingRotation,Parameters))
		{
			Pickup->Tags.AddUnique(TEXT("BuildingLoot"));
			if((Index&1)!=0)
			{
				Pickup->AmmoType=TEXT("M4A1Ammo");Pickup->ItemId=Pickup->AmmoType;Pickup->Quantity=30;
				if(UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ThirdPersonBP/Items/5_56_NATO/5_56.5_56"))){Pickup->Mesh->SetStaticMesh(Mesh);Pickup->Mesh->SetRelativeScale3D(FVector(.05f));}
				if(Pickup->MarkerText)Pickup->MarkerText->SetText(FText::FromString(TEXT("5.56 MM")));
			}
		}
	}
	if(ALootSuitcasePickup* Suitcase=GetWorld()->SpawnActor<ALootSuitcasePickup>(ALootSuitcasePickup::StaticClass(),WorldPoint(60.f,30.f,55.f),BuildingRotation,Parameters))
	{
#if WITH_EDITOR
		Suitcase->SetActorLabel(FString::Printf(TEXT("GeneratedLootSuitcase_%d"),BuildingIndex));
#endif
	}
	// Keep the former resident population and add exactly one walking brute to
	// every building. Runners retain their existing 3-4 slots.
	const int32 BaseResidentCount=FMath::Max(ZombiesPerBuilding,FloorCount*2);
	const int32 ResidentCount=BaseResidentCount+1;
	const int32 RunnerCount=FMath::Min(BaseResidentCount,3+(BuildingIndex+FloorCount)%2);
	for(int32 Index=0;Index<ResidentCount;++Index)
	{
		const int32 Level=Index%FloorCount;
		// Put residents in both rooms on every floor, never inside the stair shaft.
		// The right-room resident is immediately behind the internal door.
		const bool bRightRoom=((Index/FloorCount)&1)!=0;
		const int32 XSlot=bRightRoom?XPositions.Num()-1:FMath::Min(1,XPositions.Num()-1);
		const float X=XPositions[XSlot];
		const float Y=YPositions[(Index*2+Level)%YPositions.Num()];
		const bool bBrute=Index==0;
		const bool bRunner=!bBrute&&Index<=RunnerCount;
		TSubclassOf<AZombieCharacter> ResidentClass=bBrute?ABruteZombieCharacter::StaticClass():(bRunner?ARunnerZombieCharacter::StaticClass():AWalkerZombieCharacter::StaticClass());
		if(AZombieCharacter* Resident=GetWorld()->SpawnActor<AZombieCharacter>(ResidentClass,WorldPoint(X,Y,Level*300.f+100.f),BuildingRotation,Parameters))
		{
			Resident->SetDormant(false);
			if(!Resident->GetController())Resident->SpawnDefaultController();
			if(AZombieAIController* ResidentAI=Cast<AZombieAIController>(Resident->GetController()))
				ResidentAI->SetPatrolArea(WorldPoint(0.f,0.f,Level*300.f+100.f),FMath::Max(900.f,FMath::Max(EdgeX,EdgeY)-100.f));
		}
	}
}
