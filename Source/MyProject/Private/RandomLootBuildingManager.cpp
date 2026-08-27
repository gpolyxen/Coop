#include "RandomLootBuildingManager.h"

#include "BuildableStructure.h"
#include "LootSuitcasePickup.h"
#include "ZombieCharacter.h"
#include "ZombieAIController.h"
#include "Engine/World.h"
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
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RandomBuildingGround),false,this);float MinimumZ=MAX_flt,MaximumZ=-MAX_flt,SumZ=0.f;const FVector2D Samples[]={{0,0},{-600,-600},{-600,600},{600,-600},{600,600}};
	for(const FVector2D& Offset:Samples){FHitResult Hit;const FVector Point=Candidate+FVector(Offset.X,Offset.Y,0.f);if(!GetWorld()->LineTraceSingleByChannel(Hit,Point+FVector(0,0,3500.f),Point-FVector(0,0,5000.f),ECC_WorldStatic,Query)||Hit.ImpactNormal.Z<.9f)return false;MinimumZ=FMath::Min(MinimumZ,Hit.ImpactPoint.Z);MaximumZ=FMath::Max(MaximumZ,Hit.ImpactPoint.Z);SumZ+=Hit.ImpactPoint.Z;}
	if(MaximumZ-MinimumZ>55.f)return false;OutGround=FVector(Candidate.X,Candidate.Y,SumZ/UE_ARRAY_COUNT(Samples)+3.f);return true;
}

void ARandomLootBuildingManager::Generate()
{
	if(!HasAuthority())return;
	FRandomStream Random(FDateTime::Now().GetMillisecond()+FMath::Rand());
	int32 Spawned=0;TArray<FVector> Centers;
	for(int32 Attempt=0;Attempt<BuildingCount*24&&Spawned<BuildingCount;++Attempt)
	{
		const float Angle=Random.FRandRange(0.f,2.f*PI),Distance=Random.FRandRange(MinimumGenerationRadius,MaximumGenerationRadius);
		FVector Ground;if(!FindGround(GenerationCenter+FVector(FMath::Cos(Angle),FMath::Sin(Angle),0.f)*Distance,Ground))continue;
		bool bTooClose=false;for(const FVector& Center:Centers)if(FVector::DistSquared2D(Center,Ground)<FMath::Square(MinimumBuildingSpacing)){bTooClose=true;break;}
		if(bTooClose)continue;
		const int32 Floors=Random.RandRange(1,FMath::Max(1,MaximumFloors));
		// Generated houses need enough horizontal space for an actual stairwell and
		// rooms.  The old two-module footprint made every upper floor one narrow
		// corridor and left no valid place for another flight of stairs.
		const int32 Width=Random.FRand()<.6f?4:3;
		Centers.Add(Ground);
		SpawnBuilding(Ground,FMath::RoundToFloat(Random.FRandRange(0.f,3.f))*90.f,Spawned++,Floors,Width);
	}
	if(UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld()))Navigation->Build();
	UE_LOG(LogTemp,Display,TEXT("Generated %d random destructible loot buildings"),Spawned);
}

void ARandomLootBuildingManager::SpawnBuilding(const FVector& GroundLocation,float Yaw,int32 BuildingIndex,int32 FloorCount,int32 WidthModules)
{
	const FRotator BuildingRotation(0.f,Yaw,0.f);const FTransform Basis(BuildingRotation,GroundLocation);
	FActorSpawnParameters Parameters;Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto WorldPoint=[&](float X,float Y,float Z){return Basis.TransformPosition(FVector(X,Y,Z));};
	auto SpawnStructure=[&](UClass* Class,float X,float Y,float Z,float LocalYaw)
	{
		return GetWorld()->SpawnActor<ABuildableStructure>(Class,WorldPoint(X,Y,Z),FRotator(0.f,Yaw+LocalYaw,0.f),Parameters);
	};
	FloorCount=FMath::Max(1,FloorCount);WidthModules=FMath::Clamp(WidthModules,3,4);
	TArray<float> XPositions;for(int32 XIndex=0;XIndex<WidthModules;++XIndex)XPositions.Add((XIndex-(WidthModules-1)*.5f)*300.f);
	const float EdgeX=WidthModules*150.f;
	const bool bRoofAccess=FloorCount>1&&(BuildingIndex%2==0);
	// The stair flight is 260 cm wide.  Put its outside edge directly against the
	// left wall instead of leaving the old 20 cm slot that trapped capsules.
	const float StairX=-EdgeX+130.f;
	// Each level is made from ordinary saved/destructible build pieces. Upper
	// levels leave a deliberate stairwell tile so both players and AI have a route.
	for(int32 Level=0;Level<FloorCount;++Level)
	{
		const float Z=Level*220.f;
		for(float X:XPositions)for(float Y:{-150.f,150.f})
		{
			// A flight occupies both halves of its 3x6 m stairwell.  Keeping the
			// lower-half ceiling was the invisible obstruction that stopped both
			// players and AI halfway up the stairs.
			const bool bAboveStairwell=Level>0&&FMath::Abs(X-StairX)<=170.f;
			if(!bAboveStairwell)SpawnStructure(AWoodFloor::StaticClass(),X,Y,Z+2.f,0.f);
		}
		for(float Y:{-150.f,150.f})
		{
			SpawnStructure((Level+BuildingIndex)%3==0?AWoodWindowWall::StaticClass():AWoodWall::StaticClass(),-EdgeX,Y,Z,0.f);
			SpawnStructure((Level+BuildingIndex)%2==0?AWoodWindowWall::StaticClass():AWoodWall::StaticClass(),EdgeX,Y,Z,0.f);
		}
		for(int32 XIndex=0;XIndex<XPositions.Num();++XIndex)
		{
			const float X=XPositions[XIndex];
			SpawnStructure((XIndex+Level)%2?AWoodWindowWall::StaticClass():AWoodWall::StaticClass(),X,300.f,Z,90.f);
			// One ordinary human-sized entrance. Doors inherit the gate interaction
			// contract, so generated and player-built doors save, replicate and can be
			// opened by the same E interaction without looking like double gates.
			if(Level>0||XIndex!=WidthModules/2)SpawnStructure((XIndex+Level+1)%3==0?AWoodWindowWall::StaticClass():AWoodWall::StaticClass(),X,-300.f,Z,90.f);
			else SpawnStructure(AWoodDoor::StaticClass(),X,-300.f,Z,90.f);
		}

		// Divide every floor into two rooms. The doorway swaps sides on alternating
		// levels so residents can wait behind a real door without blocking the stair
		// landing. This also makes the layouts less like empty boxes.
		const float PartitionX=(XPositions[XPositions.Num()-2]+XPositions.Last())*.5f;
		const float DoorY=(Level&1)?150.f:-150.f;
		SpawnStructure(AWoodDoor::StaticClass(),PartitionX,DoorY,Z,0.f);
		SpawnStructure((Level+BuildingIndex)%2?AWoodWall::StaticClass():AWoodWindowWall::StaticClass(),PartitionX,-DoorY,Z,0.f);

		// A flight exists on every transition, including an optional final flight to
		// the roof. Alternating its direction keeps both landings inside the shared
		// open shaft and prevents the second flight from terminating in a wall.
		if(Level<FloorCount-1||(Level==FloorCount-1&&bRoofAccess))
			SpawnStructure(AWoodStairs::StaticClass(),StairX,0.f,Z,(Level&1)?180.f:0.f);
	}
	for(float X:XPositions)for(float Y:{-150.f,150.f})
	{
		const bool bRoofStairwell=bRoofAccess&&FMath::Abs(X-StairX)<=170.f;
		if(!bRoofStairwell)SpawnStructure(AWoodFloor::StaticClass(),X,Y,FloorCount*220.f+2.f,0.f);
	}
	if(ALootSuitcasePickup* Suitcase=GetWorld()->SpawnActor<ALootSuitcasePickup>(ALootSuitcasePickup::StaticClass(),WorldPoint(60.f,30.f,55.f),BuildingRotation,Parameters))
	{
#if WITH_EDITOR
		Suitcase->SetActorLabel(FString::Printf(TEXT("GeneratedLootSuitcase_%d"),BuildingIndex));
#endif
	}
	// Every generated floor receives at least one resident.  Some start dormant
	// and only play their wake sequence after sight, hearing or damage alerts them.
	const int32 ResidentCount=FMath::Max(ZombiesPerBuilding,FloorCount*2);
	for(int32 Index=0;Index<ResidentCount;++Index)
	{
		const int32 Level=Index%FloorCount;
		// Put residents in both rooms on every floor, never inside the stair shaft.
		// The right-room resident is immediately behind the internal door.
		const bool bRightRoom=((Index/FloorCount)&1)!=0;
		const int32 XSlot=bRightRoom?XPositions.Num()-1:FMath::Min(1,XPositions.Num()-1);
		const float X=XPositions[XSlot];
		const float Y=(Index&1)?110.f:-110.f;
		if(AZombieCharacter* Resident=GetWorld()->SpawnActor<AZombieCharacter>(AZombieCharacter::StaticClass(),WorldPoint(X,Y,Level*220.f+100.f),BuildingRotation,Parameters))
		{
			Resident->SetDormant((Index+BuildingIndex)%3==0);
			if(!Resident->GetController())Resident->SpawnDefaultController();
			if(AZombieAIController* ResidentAI=Cast<AZombieAIController>(Resident->GetController()))
				ResidentAI->SetPatrolArea(WorldPoint(0.f,0.f,Level*220.f+100.f),260.f);
		}
	}
}
