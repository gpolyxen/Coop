#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "ShooterCharacter.h"
#include "BanditCharacter.h"
#include "HealthArmorComponent.h"
#include "BuildableStructure.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AZombieAIController::AZombieAIController()
{
	PrimaryActorTick.bCanEverTick=true;
	bDiagnosticLogging=FParse::Param(FCommandLine::Get(),TEXT("CodexAIDiagnostic"));
	Senses=CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*Senses);
	Sight=CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));
	Sight->SightRadius=4500.f;
	Sight->LoseSightRadius=5600.f;
	Sight->PeripheralVisionAngleDegrees=95.f;
	Sight->SetMaxAge(ForgetAfter);
	Sight->DetectionByAffiliation.bDetectEnemies=true;
	Sight->DetectionByAffiliation.bDetectFriendlies=true;
	Sight->DetectionByAffiliation.bDetectNeutrals=true;
	Hearing=CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));
	Hearing->HearingRange=20000.f;
	Hearing->SetMaxAge(ForgetAfter);
	Hearing->DetectionByAffiliation.bDetectEnemies=true;
	Hearing->DetectionByAffiliation.bDetectFriendlies=true;
	Hearing->DetectionByAffiliation.bDetectNeutrals=true;
	Senses->ConfigureSense(*Sight);
	Senses->ConfigureSense(*Hearing);
	Senses->SetDominantSense(Sight->GetSenseImplementation());
	Senses->OnTargetPerceptionUpdated.AddDynamic(this,&AZombieAIController::OnTargetPerception);
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if(Senses)Senses->RequestStimuliListenerUpdate();
	NextPatrolTime=GetWorld()?GetWorld()->GetTimeSeconds()+FMath::FRandRange(PatrolWaitMin,PatrolWaitMax):0.;
	PatrolCenter=InPawn?InPawn->GetActorLocation():FVector::ZeroVector;
	PatrolLastProgressLocation=InPawn?InPawn->GetActorLocation():FVector::ZeroVector;
	PatrolLastProgressTime=GetWorld()?GetWorld()->GetTimeSeconds():0.;
	ResetNavigationRecovery();
	UE_LOG(LogTemp,Display,TEXT("Zombie AI %s possessed %s"),*GetName(),*GetNameSafe(InPawn));
}

void AZombieAIController::SetPatrolArea(const FVector& Center,float Radius)
{
	PatrolCenter=Center;
	PatrolAreaRadius=FMath::Max(0.f,Radius);
	bPatrolMoveActive=false;
	bSteeringPatrol=false;
	StopMovement();
	NextPatrolTime=GetWorld()?GetWorld()->GetTimeSeconds()+FMath::FRandRange(PatrolWaitMin,PatrolWaitMax):0.;
}

void AZombieAIController::ResetNavigationRecovery()
{
	ClearStairTraversal();
	ClearGateTraversal();
	bUsingDetour=false;
	DetourLocation=FVector::ZeroVector;
	DetourExpireTime=0.;
	CornerEscapeDirection=FVector::ZeroVector;
	CornerEscapeUntil=0.;
	CornerEscapeCooldownUntil=0.;
	ForceSteeringUntil=0.;
	NextPathRefreshTime=0.;
	NextDirectPathProbeTime=0.;
	bDirectPlayerPathComplete=false;
	PursuitTrailCursor=INDEX_NONE;
}

void AZombieAIController::ClearStairTraversal()
{
	if(AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn()))
	{
		if(ActiveStairs.IsValid())Zombie->GetCapsuleComponent()->IgnoreActorWhenMoving(ActiveStairs.Get(),false);
		if(UCharacterMovementComponent* Movement=Zombie->GetCharacterMovement())
		{
			if(Movement->MovementMode==MOVE_Flying)Movement->SetMovementMode(MOVE_Walking);
			Movement->bOrientRotationToMovement=true;
			Movement->Velocity.Z=0.f;
		}
	}
	ActiveStairs.Reset();
	StairEntry=FVector::ZeroVector;
	StairExit=FVector::ZeroVector;
	StairTraversalStart=FVector::ZeroVector;
	bTraversingStairs=false;
	StairTraversalStartTime=0.;
	StairTraversalDuration=1.f;
	NextStairMoveRefresh=0.;
	StairApproachExpireTime=0.;
	StairTraversalExpireTime=0.;
	bStairEntryHasNavPath=false;
	StairLastProgressLocation=FVector::ZeroVector;
	StairLastProgressTime=0.;
	LastStairRecoveryTime=-1000.;
}

void AZombieAIController::ClearGateTraversal()
{
	ActiveGate.Reset();
	GateEntry=FVector::ZeroVector;
	GateExit=FVector::ZeroVector;
	bTraversingGate=false;
	NextGateMoveRefresh=0.;
	GateApproachExpireTime=0.;
	bGateEntryHasNavPath=false;
	GateLastProgressLocation=FVector::ZeroVector;
	GateLastProgressTime=0.;
}

void AZombieAIController::AlertNearbyHorde(AActor* Actor)
{
	if(!Actor||!GetWorld())return;
	const FVector Origin=GetPawn()?GetPawn()->GetActorLocation():Actor->GetActorLocation();
	for(TActorIterator<AZombieCharacter> It(GetWorld());It;++It)
	{
		AZombieCharacter* Other=*It;
		if(!Other||Other==GetPawn()||Other->IsDead()||FVector::DistSquared2D(Origin,Other->GetActorLocation())>FMath::Square(HordeAlertRadius))continue;
		if(AZombieAIController* OtherController=Cast<AZombieAIController>(Other->GetController()))OtherController->AlertToActor(Actor);
	}
}

FVector AZombieAIController::GetVerticalRouteWaypoint(const FVector& Destination)const
{
	const AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie||!GetWorld()||FMath::Abs(Destination.Z-Zombie->GetActorLocation().Z)<115.f)return Destination;
	float Radius=0.f,HalfHeight=0.f;Zombie->GetCapsuleComponent()->GetScaledCapsuleSize(Radius,HalfHeight);
	const FVector PawnLocation=Zombie->GetActorLocation();const float FootZ=PawnLocation.Z-HalfHeight;
	const bool bGoingUp=Destination.Z>PawnLocation.Z;
	FVector Best=Destination;float BestScore=MAX_flt;
	for(TActorIterator<AWoodStairs> It(GetWorld());It;++It)
	{
		const AWoodStairs* Stairs=*It;if(!Stairs||Stairs->IsCollapsing()||Stairs->IsConstructionPreview())continue;
		TArray<FVector> Points;Stairs->GetSnapPoints(Points);if(Points.Num()<2)continue;
		const FVector Entry=bGoingUp?Points[0]:Points[1];const FVector Exit=bGoingUp?Points[1]:Points[0];
		if(FMath::Abs(Entry.Z-FootZ)>105.f)continue;
		const float Score=FVector::DistSquared2D(Exit,Destination)+FVector::DistSquared2D(PawnLocation,Entry)*.04f;
		if(Score<BestScore){BestScore=Score;Best=Entry;}
	}
	return Best;
}

bool AZombieAIController::IsAuthoredRouteBlocked(const FVector& Waypoint)const
{
	const AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());if(!Zombie||!GetWorld())return false;
	float Radius=0.f,HalfHeight=0.f;Zombie->GetCapsuleComponent()->GetScaledCapsuleSize(Radius,HalfHeight);
	const FVector Start=Zombie->GetActorLocation();FVector End=Waypoint;End.Z=Start.Z;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ZombieAuthoredRoute),false,Zombie);
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByChannel(Hits,Start,End,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(FMath::Min(35.f,Radius*.8f)),Query);
	for(const FHitResult& Hit:Hits)
	{
		const ABuildableStructure* Structure=Cast<ABuildableStructure>(Hit.GetActor());if(!Structure||Structure->IsCollapsing())continue;
		if(Structure->IsA<AWoodWall>()||Structure->IsA<AWoodWindowWall>())return true;
		if(const AWoodGate* Gate=Cast<AWoodGate>(Structure))if(!Gate->bOpen)return true;
	}
	return false;
}

bool AZombieAIController::UpdateGateTraversal(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld());
	if(!Zombie||!Navigation||!GetWorld())return false;
	const double Now=GetWorld()->GetTimeSeconds();
	float CapsuleRadius=0.f,CapsuleHalfHeight=0.f;Zombie->GetCapsuleComponent()->GetScaledCapsuleSize(CapsuleRadius,CapsuleHalfHeight);
	const FVector PawnLocation=Zombie->GetActorLocation();const float FootZ=PawnLocation.Z-CapsuleHalfHeight;
	if(!ActiveGate.IsValid())
	{
		float BestScore=MAX_flt;bool bBestReachable=false;
		for(TActorIterator<AWoodGate> It(GetWorld());It;++It)
		{
			AWoodGate* Gate=*It;
			if(!Gate||!Gate->bOpen||Gate->IsCollapsing()||Gate->IsConstructionPreview()||(Gate==LastTraversedGate.Get()&&Now<GateReuseAllowedTime))continue;
			if(FMath::Abs(Gate->GetActorLocation().Z-FootZ)>110.f)continue;
			const FVector Normal=Gate->GetActorForwardVector().GetSafeNormal2D();
			const FVector Along=Gate->GetActorRotation().RotateVector(FVector::RightVector).GetSafeNormal2D();
			const float Side=FVector::DotProduct(PawnLocation-Gate->GetActorLocation(),Normal)<0.f?-1.f:1.f;
			const float Lane=(static_cast<int32>(Zombie->GetUniqueID()%3)-1)*55.f;
			const FVector Entry=Gate->GetActorLocation()+Normal*(Side*175.f)+Along*Lane;
			const FVector Exit=Gate->GetActorLocation()-Normal*(Side*175.f)+Along*Lane;
			const float EntryDistance=FVector::DistSquared2D(PawnLocation,Entry);
			if(EntryDistance>FMath::Square(6000.f))continue;
			// Only consider a doorway lying on this route segment. Previously any open
			// door within 60 metres could become ActiveGate and hold the AI forever.
			const FVector Segment=Destination-PawnLocation;
			const float SegmentLengthSquared=Segment.SizeSquared2D();
			const float SegmentAlpha=SegmentLengthSquared>KINDA_SMALL_NUMBER
				?FMath::Clamp(FVector::DotProduct(Gate->GetActorLocation()-PawnLocation,Segment)/SegmentLengthSquared,0.f,1.f):0.f;
			const FVector ClosestOnRoute=PawnLocation+Segment*SegmentAlpha;
			if(FVector::DistSquared2D(ClosestOnRoute,Gate->GetActorLocation())>FMath::Square(520.f))continue;
			FNavLocation ProjectedEntry;
			bool bReachable=false;
			if(Navigation->ProjectPointToNavigation(Entry,ProjectedEntry,FVector(180.f,180.f,180.f)))
				if(UNavigationPath* EntryPath=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),PawnLocation,ProjectedEntry.Location,Zombie))bReachable=EntryPath->IsValid()&&!EntryPath->IsPartial();
			const float Score=EntryDistance+FVector::DistSquared2D(Exit,Destination)*.08f;
			if(Score<BestScore){BestScore=Score;ActiveGate=Gate;GateEntry=Entry;GateExit=Exit;bBestReachable=bReachable;}
		}
		if(!ActiveGate.IsValid())return false;
		StopMovement();NextGateMoveRefresh=0.;GateApproachExpireTime=Now+8.;bGateEntryHasNavPath=bBestReachable;GateLastProgressLocation=PawnLocation;GateLastProgressTime=Now;
	}
	if(!bTraversingGate)
	{
		if(Now>=GateApproachExpireTime||Now-GateLastProgressTime>=2.25)
		{
			UE_LOG(LogTemp,Warning,TEXT("AI_GATE_ABORT pawn=%s gate=%s reason=no_progress"),*GetNameSafe(Zombie),*GetNameSafe(ActiveGate.Get()));
			ClearGateTraversal();
			return false;
		}
		if(FVector::DistSquared2D(PawnLocation,GateEntry)<=FMath::Square(110.f))
		{
			StopMovement();bTraversingGate=true;
		}
		else
		{
			if(FVector::DistSquared2D(PawnLocation,GateLastProgressLocation)>FMath::Square(25.f))
			{
				GateLastProgressLocation=PawnLocation;GateLastProgressTime=Now;GateApproachExpireTime=Now+8.;
			}
			if(FVector::DistSquared2D(PawnLocation,GateEntry)<=FMath::Square(650.f))
			{
				FVector Direction=GateEntry-PawnLocation;Direction.Z=0.f;
				if(Direction.Normalize())Zombie->AddMovementInput(Direction,1.f);
				return true;
			}
			if(Now>=NextGateMoveRefresh||GetMoveStatus()==EPathFollowingStatus::Idle)
			{
				const EPathFollowingRequestResult::Type Result=MoveToLocation(GateEntry,70.f,true,true,true);NextGateMoveRefresh=Now+.5;
				if(Result==EPathFollowingRequestResult::Failed)SteerToward(GateEntry);
			}
			// A partial nav path often ends at the outside wall.  Continue local
			// collision-aware steering toward the authored gate instead of accepting
			// that partial endpoint as the final route.
			if(!bGateEntryHasNavPath||Now-GateLastProgressTime>=1.25)SteerToward(GateEntry);
			return true;
		}
	}
	if(FVector::DistSquared2D(PawnLocation,GateExit)<=FMath::Square(100.f))
	{
		LastTraversedGate=ActiveGate;GateReuseAllowedTime=Now+20.;ClearGateTraversal();NextPathRefreshTime=0.;return false;
	}
	FVector Direction=GateExit-PawnLocation;Direction.Z=0.f;if(Direction.Normalize())Zombie->AddMovementInput(Direction,1.f);
	return true;
}

bool AZombieAIController::UpdateStairTraversal(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie||!GetWorld())return false;
	float CapsuleRadius=0.f,CapsuleHalfHeight=0.f;
	Zombie->GetCapsuleComponent()->GetScaledCapsuleSize(CapsuleRadius,CapsuleHalfHeight);
	const FVector PawnLocation=Zombie->GetActorLocation();
	const float FootZ=PawnLocation.Z-CapsuleHalfHeight;

	if(!ActiveStairs.IsValid())
	{
		const float VerticalDelta=Destination.Z-PawnLocation.Z;
		if(FMath::Abs(VerticalDelta)<115.f)return false;
		const bool bGoingUp=VerticalDelta>0.f;
		UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld());
		if(!Navigation)return false;
		float BestScore=MAX_flt;bool bBestEntryHasNavPath=false;
		for(TActorIterator<AWoodStairs> It(GetWorld());It;++It)
		{
			AWoodStairs* Stairs=*It;
			if(!Stairs||Stairs->IsCollapsing()||Stairs->IsConstructionPreview())continue;
			TArray<FVector> Points;Stairs->GetSnapPoints(Points);if(Points.Num()<2)continue;
			FVector Entry=bGoingUp?Points[0]:Points[1];
			FVector Exit=bGoingUp?Points[1]:Points[0];
			const FVector Across=Stairs->GetActorForwardVector().GetSafeNormal2D();
			// Seven deterministic lanes keep a large wave from sharing one exact
			// approach point while still remaining inside the widened staircase.
			const float Lane=(static_cast<int32>(Zombie->GetUniqueID()%7)-3)*32.f;Entry+=Across*Lane;Exit+=Across*Lane;
			if(FMath::Abs(Entry.Z-FootZ)>105.f)continue;
			const float EntryDistance=FVector::DistSquared2D(PawnLocation,Entry);
			if(EntryDistance>FMath::Square(6000.f))continue;
			bool bReachableByNavigation=false;
			FNavLocation ProjectedEntry;
			if(Navigation->ProjectPointToNavigation(Entry,ProjectedEntry,FVector(180.f,180.f,180.f)))
			{
				if(UNavigationPath* EntryPath=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),PawnLocation,ProjectedEntry.Location,Zombie))
					bReachableByNavigation=EntryPath->IsValid()&&!EntryPath->IsPartial();
			}
			// Never select a geometrically close staircase on the other side of a wall.
			// That was the source of the first-floor pile-up: all controllers pressed
			// toward the same inaccessible lower landing instead of using the doorway.
			// Recast does not always connect a freshly built staircase to the floor.
			// A nearby, unobstructed authored landing is still a valid approach.
			const bool bDirectApproach=EntryDistance<=FMath::Square(1200.f)&&!IsAuthoredRouteBlocked(Entry);
			if(!bReachableByNavigation&&!bDirectApproach)continue;
			const float Score=EntryDistance+FVector::DistSquared2D(Exit,Destination)*.12f;
			if(Score<BestScore){BestScore=Score;ActiveStairs=Stairs;StairEntry=Entry;StairExit=Exit;bBestEntryHasNavPath=bReachableByNavigation;}
		}
		if(!ActiveStairs.IsValid())return false;
		StopMovement();
		// A horde must be allowed to queue through a narrow staircase without the
		// rear characters giving up and attacking the wall beside it.
		NextStairMoveRefresh=0.;StairApproachExpireTime=GetWorld()->GetTimeSeconds()+15.;StairTraversalExpireTime=0.;bStairEntryHasNavPath=bBestEntryHasNavPath;StairLastProgressLocation=PawnLocation;StairLastProgressTime=GetWorld()->GetTimeSeconds();
	}

	const double Now=GetWorld()->GetTimeSeconds();
	if(!bTraversingStairs)
	{
		// Large agents stop against the first riser before their centre reaches the
		// authored endpoint. Use a capsule-aware landing capture radius and then let
		// the deterministic stair traversal take over.
		// Brutes contact the first riser with their large capsule while their
		// centre is still roughly 3 m from the authored landing. Capture them
		// there and hand movement to the deterministic 3D traversal.
		const float EntryCaptureRadius=FMath::Clamp(CapsuleRadius+260.f,260.f,360.f);
		const bool bAtEntry=FVector::DistSquared2D(PawnLocation,StairEntry)<=FMath::Square(EntryCaptureRadius)
			&&FMath::Abs(FootZ-StairEntry.Z)<=210.f;
		if(bAtEntry)
		{
			StopMovement();
			bTraversingStairs=true;
			StairTraversalStart=PawnLocation;
			StairTraversalStartTime=Now;
			const FVector TraversalEnd=StairExit+FVector(0.f,0.f,CapsuleHalfHeight);
			StairTraversalDuration=FMath::Clamp(FVector::Distance(StairTraversalStart,TraversalEnd)/FMath::Max(225.f,Zombie->GetCharacterMovement()->MaxWalkSpeed),1.1f,2.6f);
			StairTraversalExpireTime=Now+StairTraversalDuration+1.f;
			UE_LOG(LogTemp,Display,TEXT("AI_STAIR_START pawn=%s stairs=%s entryDistance=%.0f footDelta=%.0f duration=%.2f"),
				*GetNameSafe(Zombie),*GetNameSafe(ActiveStairs.Get()),FVector::Dist2D(PawnLocation,StairEntry),FMath::Abs(FootZ-StairEntry.Z),StairTraversalDuration);
			// Runtime player-built stairs are not guaranteed to be represented by
			// Recast.  During this short authored transition the controller follows
			// the actual bottom/top endpoints in 3D.  Ignore only the stair steps;
			// walls and floors still block the zombie normally.
			Zombie->GetCapsuleComponent()->IgnoreActorWhenMoving(ActiveStairs.Get(),true);
			if(UCharacterMovementComponent* Movement=Zombie->GetCharacterMovement())
			{
				Movement->MaxFlySpeed=FMath::Max(270.f,Movement->MaxWalkSpeed);
				Movement->bOrientRotationToMovement=false;
				Movement->SetMovementMode(MOVE_Flying);
			}
		}
		else
		{
			if(FVector::DistSquared2D(PawnLocation,StairLastProgressLocation)>FMath::Square(25.f))
			{
				StairLastProgressLocation=PawnLocation;StairLastProgressTime=Now;StairApproachExpireTime=Now+10.;
			}
			// Recast cannot represent a player-built staircase reliably, especially
			// when several capsules are queued at its foot.  Once a zombie is close
			// enough, drive it to the authored entry directly and keep this state
			// instead of timing out and choosing the adjacent wall as an attack target.
			if(FVector::DistSquared2D(PawnLocation,StairEntry)<=FMath::Square(600.f))
			{
				FVector ToEntry=StairEntry-PawnLocation;ToEntry.Z=0.f;
				if(ToEntry.Normalize())
				{
					Zombie->SetActorRotation(FMath::RInterpTo(Zombie->GetActorRotation(),ToEntry.Rotation(),GetWorld()->GetDeltaSeconds(),8.f));
					Zombie->AddMovementInput(ToEntry,1.f);
				}
				return true;
			}
			if(Now>=StairApproachExpireTime){ClearStairTraversal();return false;}
			if(Now>=NextStairMoveRefresh||GetMoveStatus()==EPathFollowingStatus::Idle)
			{
				const EPathFollowingRequestResult::Type Result=MoveToLocation(StairEntry,65.f,true,true,true);
				NextStairMoveRefresh=Now+.5;
				if(Result==EPathFollowingRequestResult::Failed)SteerToward(StairEntry);
			}
			if(!bStairEntryHasNavPath||Now-StairLastProgressTime>=1.25)SteerToward(StairEntry);
			return true;
		}
	}

	const FVector TraversalTarget=StairExit+FVector(0.f,0.f,CapsuleHalfHeight);
	const float TraversalAlpha=FMath::Clamp(static_cast<float>((Now-StairTraversalStartTime)/FMath::Max(.1f,StairTraversalDuration)),0.f,1.f);
	const FVector AuthoredLocation=FMath::Lerp(StairTraversalStart,TraversalTarget,FMath::InterpEaseInOut(0.f,1.f,TraversalAlpha,1.35f));
	Zombie->SetActorLocation(AuthoredLocation,false,nullptr,ETeleportType::TeleportPhysics);
	FVector Facing=StairExit-StairEntry;Facing.Z=0.f;
	if(Facing.Normalize())Zombie->SetActorRotation(FMath::RInterpConstantTo(Zombie->GetActorRotation(),Facing.Rotation(),GetWorld()->GetDeltaSeconds(),480.f));
	if(TraversalAlpha>=1.f||Now>=StairTraversalExpireTime)
	{
		ClearStairTraversal();
		NextPathRefreshTime=0.;
		return false;
	}
	return true;
}

void AZombieAIController::AlertToActor(AActor* Actor)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	const AShooterCharacter* Player=Cast<AShooterCharacter>(Actor);
	const ABanditCharacter* Bandit=Cast<ABanditCharacter>(Actor);
	if((!Player&&!Bandit)||(Player&&Player->IsDead())||(Bandit&&Bandit->IsDead()))return;
	if(Zombie)Zombie->WakeUp();
	// A noise wakes nearby zombies, but never steals aggro from a target that the
	// zombie can currently see. Otherwise distant gunfire made melee encounters
	// feel random whenever players and bandits fought in the same area.
	AActor* Current=Target.Get();
	if(Current&&Current!=Actor&&LineOfSightTo(Current))
	{
		LastKnownLocation=Current->GetActorLocation();
		LastStimulus=GetWorld()->GetTimeSeconds();
		return;
	}
	if(Current&&Current!=Actor&&GetPawn()&&
		FVector::DistSquared(GetPawn()->GetActorLocation(),Actor->GetActorLocation())>=
		FVector::DistSquared(GetPawn()->GetActorLocation(),Current->GetActorLocation()))return;
	const bool bNewTarget=Target.Get()!=Actor;
	Target=Actor;
	LastKnownLocation=Actor->GetActorLocation();
	LastStimulus=GetWorld()->GetTimeSeconds();
	bPatrolMoveActive=false;
	bSteeringPatrol=false;
	// A shot must interrupt patrol immediately even if this player was already
	// remembered from an older stimulus.
	NextPathRefreshTime=0.;
	// Repeated gunshots refresh awareness but must not erase an in-progress gate
	// or stair traversal.  That reset made a firing player continuously send the
	// horde back to the beginning of its route.
	if(bNewTarget)
	{
		LastChaseProgressLocation=GetPawn()?GetPawn()->GetActorLocation():FVector::ZeroVector;
		LastChaseProgressTime=GetWorld()->GetTimeSeconds();
		ResetNavigationRecovery();
	}
}

void AZombieAIController::OnTargetPerception(AActor* Actor,FAIStimulus Stimulus)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	const AShooterCharacter* Player=Cast<AShooterCharacter>(Actor);
	const ABanditCharacter* Bandit=Cast<ABanditCharacter>(Actor);
	if((!Player&&!Bandit)||Actor==GetPawn()||(Player&&Player->IsDead())||(Bandit&&Bandit->IsDead()))return;
	if(Stimulus.WasSuccessfullySensed())
	{
		if(Zombie)Zombie->WakeUp();
		AActor* Current=Target.Get();
		if(Current&&Current!=Actor&&LineOfSightTo(Current))
		{
			LastKnownLocation=Current->GetActorLocation();
			LastStimulus=GetWorld()->GetTimeSeconds();
			return;
		}
		const bool bNewActorVisible=LineOfSightTo(Actor);
		if(Current&&Current!=Actor&&!bNewActorVisible&&GetPawn()&&
			FVector::DistSquared(GetPawn()->GetActorLocation(),Actor->GetActorLocation())>=
			FVector::DistSquared(GetPawn()->GetActorLocation(),Current->GetActorLocation()))return;
		const bool bNewTarget=Target.Get()!=Actor;
		Target=Actor;
		LastKnownLocation=Stimulus.StimulusLocation.IsNearlyZero()?Actor->GetActorLocation():Stimulus.StimulusLocation;
		LastStimulus=GetWorld()->GetTimeSeconds();
		if(bNewTarget)
		{
			bPatrolMoveActive=false;
			bSteeringPatrol=false;
			LastChaseProgressLocation=GetPawn()?GetPawn()->GetActorLocation():FVector::ZeroVector;
			LastChaseProgressTime=GetWorld()->GetTimeSeconds();
			ResetNavigationRecovery();
			UE_LOG(LogTemp,Display,TEXT("Zombie perception acquired %s"),*GetNameSafe(Actor));
			AlertNearbyHorde(Actor);
		}
	}
}

void AZombieAIController::TryStartPatrol()
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld());
	if(!Zombie||!Navigation||Zombie->IsDormant())return;
	const float EffectiveRadius=PatrolAreaRadius>0.f?PatrolAreaRadius:PatrolRadius;
	const FVector SearchCenter=PatrolAreaRadius>0.f?PatrolCenter:Zombie->GetActorLocation();
	FNavLocation PatrolPoint;
	if(Navigation->GetRandomReachablePointInRadius(SearchCenter,EffectiveRadius,PatrolPoint))
	{
		bPatrolMoveActive=MoveToLocation(PatrolPoint.Location,PatrolAcceptanceRadius,true,true,true)!=EPathFollowingRequestResult::Failed;
		bSteeringPatrol=false;
		PatrolLastProgressLocation=Zombie->GetActorLocation();
		PatrolLastProgressTime=GetWorld()->GetTimeSeconds();
		if(!bPatrolMoveActive)NextPatrolTime=GetWorld()->GetTimeSeconds()+.35f;
	}
	else
	{
		const FVector2D Direction=FMath::RandPointInCircle(EffectiveRadius);
		SteeringPatrolLocation=SearchCenter+FVector(Direction.X,Direction.Y,0.f);
		bSteeringPatrol=true;
		bPatrolMoveActive=true;
		PatrolLastProgressLocation=Zombie->GetActorLocation();
		PatrolLastProgressTime=GetWorld()->GetTimeSeconds();
	}
}

bool AZombieAIController::TryJumpObstacle(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie||!GetWorld())return false;
	UCharacterMovementComponent* Movement=Zombie->GetCharacterMovement();
	UCapsuleComponent* Capsule=Zombie->GetCapsuleComponent();
	const double Now=GetWorld()->GetTimeSeconds();
	if(!Movement||!Capsule||Movement->IsFalling()||!Movement->IsMovingOnGround()||Now-LastJumpTime<JumpCooldown)return false;

	FVector TravelDirection=Zombie->GetVelocity();
	TravelDirection.Z=0.f;
	if(TravelDirection.SizeSquared2D()<FMath::Square(35.f))
	{
		TravelDirection=Destination-Zombie->GetActorLocation();
		TravelDirection.Z=0.f;
	}
	if(!TravelDirection.Normalize())return false;

	float CapsuleRadius=0.f;
	float CapsuleHalfHeight=0.f;
	Capsule->GetScaledCapsuleSize(CapsuleRadius,CapsuleHalfHeight);
	const FVector ActorLocation=Zombie->GetActorLocation();
	const float GroundZ=ActorLocation.Z-CapsuleHalfHeight;
	const float ProbeRadius=FMath::Clamp(CapsuleRadius*.55f,18.f,28.f);
	const FVector ProbeStart(ActorLocation.X,ActorLocation.Y,GroundZ+Movement->MaxStepHeight+ProbeRadius*.75f);
	const FVector ProbeEnd=ProbeStart+TravelDirection*JumpProbeDistance;

	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ZombieJumpProbe),false,Zombie);
	if(Target.IsValid())Query.AddIgnoredActor(Target.Get());

	FHitResult ObstacleHit;
	if(!GetWorld()->SweepSingleByObjectType(ObstacleHit,ProbeStart,ProbeEnd,FQuat::Identity,ObjectTypes,FCollisionShape::MakeSphere(ProbeRadius),Query))return false;
	if(ObstacleHit.bStartPenetrating||ObstacleHit.ImpactNormal.Z>=Movement->GetWalkableFloorZ())return false;

	const FVector TopProbePoint=ObstacleHit.ImpactPoint+TravelDirection*(ProbeRadius+8.f);
	const FVector TopTraceStart(TopProbePoint.X,TopProbePoint.Y,GroundZ+MaximumJumpObstacleHeight+60.f);
	const FVector TopTraceEnd(TopProbePoint.X,TopProbePoint.Y,GroundZ+1.f);
	FHitResult TopHit;
	if(!GetWorld()->LineTraceSingleByObjectType(TopHit,TopTraceStart,TopTraceEnd,ObjectTypes,Query))return false;
	const float ObstacleHeight=TopHit.ImpactPoint.Z-GroundZ;
	if(ObstacleHeight<=FMath::Max(MinimumJumpObstacleHeight,Movement->MaxStepHeight+2.f)||ObstacleHeight>MaximumJumpObstacleHeight)return false;
	if(TopHit.ImpactNormal.Z<Movement->GetWalkableFloorZ())return false;

	const float Gravity=FMath::Abs(Movement->GetGravityZ());
	if(Gravity<=KINDA_SMALL_NUMBER)return false;
	const float MaximumBallisticRise=FMath::Square(Movement->JumpZVelocity)/(2.f*Gravity);
	const float RequiredRise=ObstacleHeight+JumpClearance;
	if(RequiredRise>MaximumBallisticRise*.9f)return false;

	const FCollisionShape ClearanceCapsule=FCollisionShape::MakeCapsule(FMath::Max(8.f,CapsuleRadius-4.f),FMath::Max(12.f,CapsuleHalfHeight-6.f));
	FHitResult ClearanceHit;
	const FVector RaisedLocation=ActorLocation+FVector(0.f,0.f,RequiredRise);
	if(GetWorld()->SweepSingleByObjectType(ClearanceHit,ActorLocation,RaisedLocation,FQuat::Identity,ObjectTypes,ClearanceCapsule,Query))return false;

	const float LandingDistance=JumpProbeDistance*2.f;
	const FVector RaisedEnd=RaisedLocation+TravelDirection*LandingDistance;
	if(GetWorld()->SweepSingleByObjectType(ClearanceHit,RaisedLocation,RaisedEnd,FQuat::Identity,ObjectTypes,ClearanceCapsule,Query))return false;

	const FVector LandingProbe=ActorLocation+TravelDirection*LandingDistance;
	const FVector LandingTraceStart(LandingProbe.X,LandingProbe.Y,GroundZ+MaximumBallisticRise+60.f);
	const FVector LandingTraceEnd(LandingProbe.X,LandingProbe.Y,GroundZ-MaximumLandingDrop);
	FHitResult LandingHit;
	if(!GetWorld()->LineTraceSingleByObjectType(LandingHit,LandingTraceStart,LandingTraceEnd,ObjectTypes,Query))return false;
	const float LandingHeight=LandingHit.ImpactPoint.Z-GroundZ;
	if(LandingHeight>MaximumJumpObstacleHeight||LandingHeight<-MaximumLandingDrop||LandingHit.ImpactNormal.Z<Movement->GetWalkableFloorZ())return false;

	StopMovement();
	Movement->StopMovementImmediately();
	JumpTravelDirection=TravelDirection;
	LastJumpTime=Now;
	bUsingDetour=false;
	NextPathRefreshTime=Now+PathRefreshInterval;
	Zombie->LaunchCharacter(TravelDirection*JumpForwardVelocity+FVector(0.f,0.f,Movement->JumpZVelocity),true,true);
	UE_LOG(LogTemp,Display,TEXT("Zombie %s jumps over %.0f cm obstacle"),*GetNameSafe(Zombie),ObstacleHeight);
	return true;
}

bool AZombieAIController::TryStartDetour(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	UNavigationSystemV1* Navigation=UNavigationSystemV1::GetCurrent(GetWorld());
	if(!Zombie||!Navigation)return false;

	FVector Forward=Destination-Zombie->GetActorLocation();
	Forward.Z=0.f;
	if(!Forward.Normalize())return false;
	const FVector Right=FVector::CrossProduct(FVector::UpVector,Forward).GetSafeNormal();
	const FVector Start=Zombie->GetActorLocation();
	FNavLocation ProjectedDestination;
	if(!Navigation->ProjectPointToNavigation(Destination,ProjectedDestination,FVector(220.f,220.f,350.f)))return false;
	const float Side=DetourSearchRadius;
	const float Ahead=DetourSearchRadius*.55f;
	TArray<FVector> Candidates;
	Candidates.Reserve(6);
	Candidates.Add(Start+Forward*Ahead+Right*Side);
	Candidates.Add(Start+Forward*Ahead-Right*Side);
	Candidates.Add(Start+Right*Side);
	Candidates.Add(Start-Right*Side);
	Candidates.Add(Start+Forward*DetourSearchRadius+Right*Side*.55f);
	Candidates.Add(Start+Forward*DetourSearchRadius-Right*Side*.55f);

	bool bFoundCandidate=false;
	float BestScore=MAX_flt;
	FVector BestLocation=FVector::ZeroVector;
	for(const FVector& Candidate:Candidates)
	{
		FNavLocation Projected;
		if(!Navigation->ProjectPointToNavigation(Candidate,Projected,FVector(180.f,180.f,300.f)))continue;
		UNavigationPath* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),Start,Projected.Location,Zombie);
		if(!Path||!Path->IsValid()||Path->IsPartial())continue;
		UNavigationPath* DestinationPath=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),Projected.Location,ProjectedDestination.Location,Zombie);
		if(!DestinationPath||!DestinationPath->IsValid()||DestinationPath->IsPartial())continue;
		const float Score=FVector::DistSquared2D(Projected.Location,Destination)+FVector::DistSquared2D(Projected.Location,Start)*.2f;
		if(Score<BestScore)
		{
			BestScore=Score;
			BestLocation=Projected.Location;
			bFoundCandidate=true;
		}
	}
	if(!bFoundCandidate)return false;

	const EPathFollowingRequestResult::Type MoveResult=MoveToLocation(BestLocation,PatrolAcceptanceRadius,true,true,true);
	if(MoveResult==EPathFollowingRequestResult::Failed)return false;
	bUsingDetour=true;
	DetourLocation=BestLocation;
	DetourExpireTime=GetWorld()->GetTimeSeconds()+DetourTimeout;
	NextPathRefreshTime=DetourExpireTime;
	UE_LOG(LogTemp,Display,TEXT("Zombie %s uses a NavMesh detour"),*GetNameSafe(Zombie));
	return true;
}

bool AZombieAIController::StartCornerEscape(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie||!GetWorld())return false;
	const double Now=GetWorld()->GetTimeSeconds();
	if(Now<CornerEscapeCooldownUntil)return false;
	// Do not visibly turn away from a nearby player. Local collision steering can
	// slide around a door frame while preserving pursuit direction; the full
	// back-out manoeuvre is reserved for hidden/remote targets and real corners.
	if(Target.IsValid()&&LineOfSightTo(Target.Get())
		&&FVector::DistSquared2D(Zombie->GetActorLocation(),Target->GetActorLocation())<=FMath::Square(1600.f))return false;
	const FVector Start=Zombie->GetActorLocation()+FVector(0.f,0.f,55.f);
	FVector ToGoal=Destination-Zombie->GetActorLocation();ToGoal.Z=0.f;ToGoal.Normalize();
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ZombieCornerEscape),false,Zombie);
	if(Target.IsValid())Query.AddIgnoredActor(Target.Get());
	float BestScore=-MAX_flt;
	FVector BestDirection=FVector::ZeroVector;
	// Test the full circle. Clearance dominates the score; goal alignment only
	// breaks ties so an agent in a corner is willing to back out before chasing.
	for(int32 Index=0;Index<12;++Index)
	{
		const FVector Direction=FVector::ForwardVector.RotateAngleAxis(Index*30.f,FVector::UpVector);
		FHitResult Hit;
		const FVector End=Start+Direction*280.f;
		const bool bHit=GetWorld()->SweepSingleByChannel(Hit,Start,End,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(38.f),Query);
		const float Clearance=bHit?FVector::Dist2D(Start,Hit.Location):280.f;
		const float Score=Clearance+FVector::DotProduct(Direction,ToGoal)*35.f;
		if(Clearance>=95.f&&Score>BestScore){BestScore=Score;BestDirection=Direction;}
	}
	if(BestDirection.IsNearlyZero())return false;
	StopMovement();
	bUsingDetour=false;
	CornerEscapeDirection=BestDirection;
	CornerEscapeUntil=Now+.85;
	CornerEscapeCooldownUntil=Now+4.;
	UE_LOG(LogTemp,Warning,TEXT("AI_STUCK_RECOVERY pawn=%s action=corner_escape direction=%s target=%s"),
		*GetNameSafe(Zombie),*BestDirection.ToCompactString(),*GetNameSafe(Target.Get()));
	return true;
}

bool AZombieAIController::TryAttackBlockingStructure(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie||!GetWorld()||GetWorld()->GetTimeSeconds()-LastChaseProgressTime<StuckRecoveryDelay)return false;
	FVector Direction=GetImmediateMoveDestination()-Zombie->GetActorLocation();Direction.Z=0.f;
	if(Direction.SizeSquared2D()<FMath::Square(20.f)){Direction=Destination-Zombie->GetActorLocation();Direction.Z=0.f;}
	if(!Direction.Normalize())return false;
	const FVector Start=Zombie->GetActorLocation()+FVector(0.f,0.f,60.f),End=Start+Direction*FMath::Max(180.f,Zombie->AttackRange+55.f);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ZombieBlockingStructure),false,Zombie);if(Target.IsValid())Query.AddIgnoredActor(Target.Get());
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByChannel(Hits,Start,End,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(48.f),Query);
	for(const FHitResult& Hit:Hits)
	{
		ABuildableStructure* Structure=Cast<ABuildableStructure>(Hit.GetActor());
		if(!Structure||Structure->IsCollapsing()||Structure->IsA<AWoodStairs>()||Structure->IsA<AWoodFloor>()||Structure->IsA<AWallTorch>())continue;
		if(Structure->ActorHasTag(TEXT("GeneratedBuilding"))&&(Structure->IsA<AWoodWall>()||Structure->IsA<AWoodWindowWall>()))continue;
		if(const AWoodGate* Gate=Cast<AWoodGate>(Structure))if(Gate->bOpen)continue;
		StopMovement();
		return Zombie->TryAttack(Structure);
	}
	ABuildableStructure* Nearest=nullptr;float BestDistanceSquared=FMath::Square(Zombie->AttackRange+85.f);
	for(TActorIterator<ABuildableStructure> It(GetWorld());It;++It)
	{
		ABuildableStructure* Structure=*It;if(!Structure||Structure->IsCollapsing()||Structure->IsA<AWoodStairs>()||Structure->IsA<AWoodFloor>()||Structure->IsA<AWallTorch>())continue;
		if(Structure->ActorHasTag(TEXT("GeneratedBuilding"))&&(Structure->IsA<AWoodWall>()||Structure->IsA<AWoodWindowWall>()))continue;
		if(const AWoodGate* Gate=Cast<AWoodGate>(Structure))if(Gate->bOpen)continue;
		FVector ToStructure=Structure->GetActorLocation()-Zombie->GetActorLocation();ToStructure.Z=0.f;const float DistanceSquared=ToStructure.SizeSquared2D();
		if(DistanceSquared<BestDistanceSquared&&FVector::DotProduct(Direction,ToStructure.GetSafeNormal2D())>.15f){BestDistanceSquared=DistanceSquared;Nearest=Structure;}
	}
	if(Nearest){StopMovement();return Zombie->TryAttack(Nearest);}
	return false;
}

void AZombieAIController::SteerToward(const FVector& Destination)
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie)return;
	if(TryJumpObstacle(Destination))return;
	FVector DesiredDirection=Destination-Zombie->GetActorLocation();
	DesiredDirection.Z=0.f;
	if(!DesiredDirection.Normalize())return;

	const FVector TraceStart=Zombie->GetActorLocation()+FVector(0.f,0.f,50.f);
	const FVector TraceEnd=TraceStart+DesiredDirection*180.f;
	FHitResult Obstacle;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ZombieSteering),false,Zombie);
	// The pursued player is the destination, never an obstacle to avoid. Without
	// this exclusion the close-range probe made zombies sharply sidestep through
	// a doorway just before reaching the player, then turn back on the next tick.
	if(Target.IsValid())Query.AddIgnoredActor(Target.Get());
	if(GetWorld()->SweepSingleByChannel(Obstacle,TraceStart,TraceEnd,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(42.f),Query))
	{
		if(ABuildableStructure* Structure=Cast<ABuildableStructure>(Obstacle.GetActor()))
		{
			const bool bGeneratedPermanentWall=Structure->ActorHasTag(TEXT("GeneratedBuilding"))
				&&(Structure->IsA<AWoodWall>()||Structure->IsA<AWoodWindowWall>());
			if(!bGeneratedPermanentWall&&!Structure->IsCollapsing()&&FVector::Dist2D(Zombie->GetActorLocation(),Obstacle.ImpactPoint)<=Zombie->AttackRange+55.f)
			{
				StopMovement();
				Zombie->TryAttack(Structure);
				return;
			}
		}
		FVector SlideDirection=FVector::VectorPlaneProject(DesiredDirection,Obstacle.ImpactNormal);
		SlideDirection.Z=0.f;
		if(SlideDirection.Normalize())
		{
			// Probe both possible turns and select the side with free space instead
			// of repeatedly choosing the same wall-facing slide direction.
			const FVector Left=DesiredDirection.RotateAngleAxis(-58.f,FVector::UpVector);
			const FVector Right=DesiredDirection.RotateAngleAxis(58.f,FVector::UpVector);
			const bool bLeftBlocked=GetWorld()->SweepTestByChannel(TraceStart,TraceStart+Left*150.f,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(36.f),Query);
			const bool bRightBlocked=GetWorld()->SweepTestByChannel(TraceStart,TraceStart+Right*150.f,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(36.f),Query);
			if(bLeftBlocked!=bRightBlocked)DesiredDirection=bLeftBlocked?Right:Left;
			else DesiredDirection=SlideDirection;
		}
		else DesiredDirection=DesiredDirection.RotateAngleAxis(58.f,FVector::UpVector);
	}
	Zombie->AddMovementInput(DesiredDirection,1.f);
}

void AZombieAIController::TryAcquireVisibleTarget()
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie||!Sight)return;
	const FVector EyesLocation=Zombie->GetActorLocation()+FVector(0.f,0.f,55.f);
	const float MinimumFacingDot=FMath::Cos(FMath::DegreesToRadians(Sight->PeripheralVisionAngleDegrees));
	AActor* BestTarget=nullptr;
	float BestDistanceSquared=FMath::Square(Sight->SightRadius);
	int32 PlayerCount=0;
	for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)
	{
		AShooterCharacter* Player=*It;
		++PlayerCount;
		if(!Player||Player->IsDead())continue;
		FVector ToPlayer=Player->GetActorLocation()-EyesLocation;
		const float DistanceSquared=ToPlayer.SizeSquared();
		ToPlayer.Z=0.f;
		if(!bLoggedInitialSightScan)UE_LOG(LogTemp,Display,TEXT("Zombie sight scan: player=%s distance=%.0f facing=%.2f line-of-sight=%s"),*GetNameSafe(Player),FMath::Sqrt(DistanceSquared),FVector::DotProduct(Zombie->GetActorForwardVector(),ToPlayer.GetSafeNormal()),LineOfSightTo(Player,EyesLocation)?TEXT("true"):TEXT("false"));
		if(DistanceSquared>BestDistanceSquared||ToPlayer.IsNearlyZero())continue;
		const bool bInsideProximityRadius=DistanceSquared<=FMath::Square(ProximityAwarenessRadius);
		if(!bInsideProximityRadius&&FVector::DotProduct(Zombie->GetActorForwardVector(),ToPlayer.GetSafeNormal())<MinimumFacingDot)continue;
		if(!LineOfSightTo(Player,EyesLocation))continue;
		BestTarget=Player;
		BestDistanceSquared=DistanceSquared;
	}
	for(TActorIterator<ABanditCharacter> It(GetWorld());It;++It)
	{
		ABanditCharacter* Bandit=*It;
		if(!Bandit||Bandit->IsDead())continue;
		FVector ToBandit=Bandit->GetActorLocation()-EyesLocation;
		const float DistanceSquared=ToBandit.SizeSquared();
		ToBandit.Z=0.f;
		if(DistanceSquared>BestDistanceSquared||ToBandit.IsNearlyZero())continue;
		const bool bInsideProximityRadius=DistanceSquared<=FMath::Square(ProximityAwarenessRadius);
		if(!bInsideProximityRadius&&FVector::DotProduct(Zombie->GetActorForwardVector(),ToBandit.GetSafeNormal())<MinimumFacingDot)continue;
		if(!LineOfSightTo(Bandit,EyesLocation))continue;
		BestTarget=Bandit;
		BestDistanceSquared=DistanceSquared;
	}
	if(!bLoggedInitialSightScan)
	{
		UE_LOG(LogTemp,Display,TEXT("Zombie initial sight scan found %d shooter character(s)"),PlayerCount);
		bLoggedInitialSightScan=true;
	}
	if(BestTarget)
	{
		Zombie->WakeUp();
		Target=BestTarget;
		LastKnownLocation=BestTarget->GetActorLocation();
		LastStimulus=GetWorld()->GetTimeSeconds();
		bPatrolMoveActive=false;
		bSteeringPatrol=false;
		LastChaseProgressLocation=Zombie->GetActorLocation();
		LastChaseProgressTime=GetWorld()->GetTimeSeconds();
		ResetNavigationRecovery();
		AlertNearbyHorde(BestTarget);
		UE_LOG(LogTemp,Display,TEXT("Zombie %s acquired %s by sight"),*GetNameSafe(Zombie),*GetNameSafe(BestTarget));
	}
}

void AZombieAIController::AcquireNearestLivingPlayer()
{
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie)return;
	AShooterCharacter* NearestPlayer=nullptr;
	float NearestDistanceSquared=MAX_flt;
	for(TActorIterator<AShooterCharacter> It(GetWorld());It;++It)
	{
		AShooterCharacter* Player=*It;
		if(!Player||Player->IsDead())continue;
		const float DistanceSquared=FVector::DistSquared(Zombie->GetActorLocation(),Player->GetActorLocation());
		if(DistanceSquared<NearestDistanceSquared)
		{
			NearestDistanceSquared=DistanceSquared;
			NearestPlayer=Player;
		}
	}
	if(!NearestPlayer)return;
	const bool bChangedTarget=Target.Get()!=NearestPlayer;
	if(Zombie->IsDormant())Zombie->WakeUp();
	Target=NearestPlayer;
	LastKnownLocation=NearestPlayer->GetActorLocation();
	LastStimulus=GetWorld()->GetTimeSeconds();
	if(bChangedTarget)
	{
		bPatrolMoveActive=false;
		bSteeringPatrol=false;
		LastChaseProgressLocation=Zombie->GetActorLocation();
		LastChaseProgressTime=LastStimulus;
		ResetNavigationRecovery();
	}
}

void AZombieAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AZombieCharacter* Zombie=Cast<AZombieCharacter>(GetPawn());
	if(!Zombie||Zombie->IsDead()){StopMovement();return;}
	const double Now=GetWorld()->GetTimeSeconds();
	if(bDiagnosticLogging&&Now>=NextDiagnosticLogTime)
	{
		NextDiagnosticLogTime=Now+1.;
		const float TargetDistance=Target.IsValid()?FVector::Dist(Zombie->GetActorLocation(),Target->GetActorLocation()):-1.f;
		const bool bHasLOS=Target.IsValid()&&LineOfSightTo(Target.Get());
		UE_LOG(LogTemp,Display,TEXT("AI_DIAG pawn=%s loc=%s vel=%.0f target=%s dist=%.0f los=%d pathComplete=%d move=%d stuckFor=%.1f trail=%d detour=%d stairs=%d/%d gate=%d/%d steeringUntil=%.1f"),
			*GetNameSafe(Zombie),*Zombie->GetActorLocation().ToCompactString(),Zombie->GetVelocity().Size2D(),
			*GetNameSafe(Target.Get()),TargetDistance,bHasLOS?1:0,bDirectPlayerPathComplete?1:0,static_cast<int32>(GetMoveStatus()),
			Now-LastChaseProgressTime,PursuitTrailCursor,bUsingDetour?1:0,ActiveStairs.IsValid()?1:0,bTraversingStairs?1:0,
			ActiveGate.IsValid()?1:0,bTraversingGate?1:0,FMath::Max(0.,ForceSteeringUntil-Now));
	}
	if(Target.IsValid()&&Now-LastChaseProgressTime>=2.25&&Now>=NextAutomaticStuckLogTime)
	{
		NextAutomaticStuckLogTime=Now+2.;
		UE_LOG(LogTemp,Warning,TEXT("AI_STUCK pawn=%s loc=%s vel=%.0f target=%s targetLoc=%s dist=%.0f los=%d pathComplete=%d move=%d stuckFor=%.1f trail=%d detour=%d stairs=%d/%d gate=%d/%d"),
			*GetNameSafe(Zombie),*Zombie->GetActorLocation().ToCompactString(),Zombie->GetVelocity().Size2D(),*GetNameSafe(Target.Get()),
			*Target->GetActorLocation().ToCompactString(),FVector::Dist(Zombie->GetActorLocation(),Target->GetActorLocation()),LineOfSightTo(Target.Get())?1:0,
			bDirectPlayerPathComplete?1:0,static_cast<int32>(GetMoveStatus()),Now-LastChaseProgressTime,PursuitTrailCursor,bUsingDetour?1:0,
			ActiveStairs.IsValid()?1:0,bTraversingStairs?1:0,ActiveGate.IsValid()?1:0,bTraversingGate?1:0);
	}
	if(Now<CornerEscapeUntil)
	{
		Zombie->AddMovementInput(CornerEscapeDirection,1.f);
		return;
	}
	if(CornerEscapeUntil>0.)
	{
		CornerEscapeUntil=0.;
		CornerEscapeDirection=FVector::ZeroVector;
		LastChaseProgressLocation=Zombie->GetActorLocation();
		LastChaseProgressTime=Now;
		NextPathRefreshTime=0.;
	}
	// Zombies are the permanent pressure in this game. Perception still drives
	// reactions and horde alerts, but it must never be the single point of failure:
	// every living zombie periodically locks onto the nearest living player.
	if(Now>=NextForcedPlayerScanTime)
	{
		AcquireNearestLivingPlayer();
		NextForcedPlayerScanTime=Now+.35;
	}
	if(Zombie->IsDormant()){StopMovement();bPatrolMoveActive=false;bSteeringPatrol=false;return;}
	if(Zombie->IsAttacking()||Zombie->IsHitReacting()){StopMovement();return;}
	UCharacterMovementComponent* Movement=Zombie->GetCharacterMovement();
	if(Movement&&Movement->IsFalling())
	{
		if(!JumpTravelDirection.IsNearlyZero())Zombie->AddMovementInput(JumpTravelDirection,1.f);
		return;
	}
	if(!JumpTravelDirection.IsNearlyZero())
	{
		JumpTravelDirection=FVector::ZeroVector;
		NextPathRefreshTime=0.;
	}

	if(Target.IsValid())
	{
		if(UHealthArmorComponent* TargetHealth=Target->FindComponentByClass<UHealthArmorComponent>())
			if(TargetHealth->IsDead())
			{
				Target.Reset();
				ResetNavigationRecovery();
			}
		if(ABanditCharacter* Bandit=Cast<ABanditCharacter>(Target.Get()))
			if(Bandit->IsDead())
			{
				Target.Reset();
				ResetNavigationRecovery();
			}
	}
	if(!Target.IsValid())TryAcquireVisibleTarget();

	if(Target.IsValid())
	{
		const bool bCanSeeTarget=LineOfSightTo(Target.Get());
		// A living player remains the authoritative destination even through walls.
		// The breadcrumb route below supplies the legal path through the same doors,
		// corridors and stairs that the player actually used.
		if(Cast<AShooterCharacter>(Target.Get())||bCanSeeTarget)
		{
			LastKnownLocation=Target->GetActorLocation();
			LastStimulus=Now;
		}
		if(Now-LastStimulus>ForgetAfter)
		{
			PatrolCenter=LastKnownLocation;
			PatrolAreaRadius=1100.f;
			Target.Reset();
			ResetNavigationRecovery();
			StopMovement();
			NextPatrolTime=Now+FMath::FRandRange(PatrolWaitMin,PatrolWaitMax);
		}
		else
		{
			const float Distance=FVector::Dist(Zombie->GetActorLocation(),Target->GetActorLocation());
			if(FVector::DistSquared2D(Zombie->GetActorLocation(),LastChaseProgressLocation)>FMath::Square(25.f))
			{
				LastChaseProgressLocation=Zombie->GetActorLocation();
				LastChaseProgressTime=Now;
			}
			bool bMeleeSpaceClear=false;
			if(Distance<=Zombie->AttackRange)
			{
				FCollisionQueryParams MeleeQuery(SCENE_QUERY_STAT(ZombieMeleeAccess),false,Zombie);
				MeleeQuery.AddIgnoredActor(Target.Get());
				const FVector Start=Zombie->GetActorLocation()+FVector(0.f,0.f,45.f);
				const FVector End=Target->GetActorLocation()+FVector(0.f,0.f,45.f);
				bMeleeSpaceClear=!GetWorld()->SweepTestByChannel(Start,End,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(42.f),MeleeQuery);
			}
			if(bMeleeSpaceClear&&Distance<=Zombie->AttackRange)
			{
				StopMovement();
				Zombie->TryAttack(Target.Get());
			}
			else
			{
				const FVector Destination=bCanSeeTarget?Target->GetActorLocation():LastKnownLocation;
				const bool bNeedsVerticalRoute=FMath::Abs(Destination.Z-Zombie->GetActorLocation().Z)>=115.f;
			if(Now>=NextDirectPathProbeTime)
			{
				bDirectPlayerPathComplete=false;
				if(UNavigationPath* DirectPath=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),Zombie->GetActorLocation(),Destination,Zombie))
					bDirectPlayerPathComplete=DirectPath->IsValid()&&!DirectPath->IsPartial();
				NextDirectPathProbeTime=Now+.4;
			}
				// A stair is only accepted when its entry is reachable on the zombie's
				// current navigation island. Once selected, keep its authored traversal
				// state until the next storey is reached.
				if(ActiveStairs.IsValid()&&UpdateStairTraversal(Destination))return;
				if(bNeedsVerticalRoute&&UpdateStairTraversal(Destination))return;

				FVector RouteWaypoint=GetVerticalRouteWaypoint(Destination);
				bool bFollowingPlayerTrail=false;
				const bool bCloseToPlayer=bCanSeeTarget
					&&FVector::DistSquared2D(Zombie->GetActorLocation(),Destination)<=FMath::Square(1600.f)
					&&FMath::Abs(Destination.Z-Zombie->GetActorLocation().Z)<115.f;
				if((bNeedsVerticalRoute||!bCanSeeTarget||!bDirectPlayerPathComplete)&&!bCloseToPlayer)
				{
					// Follow the player's ordered route whenever walls hide him as well as
					// between storeys. This guides zombies through the actual room doorway
					// instead of steering into the wall nearest to the player.
					FVector TrailWaypoint;
					AShooterCharacter* PursuedPlayer=Cast<AShooterCharacter>(Target.Get());
					if(PursuedPlayer&&PursuedPlayer->GetPursuitTrailWaypoint(Zombie->GetActorLocation(),PursuitTrailCursor,TrailWaypoint))
					{
						RouteWaypoint=TrailWaypoint;
						bFollowingPlayerTrail=true;
					}
				}
				const bool bRouteBlocked=IsAuthoredRouteBlocked(RouteWaypoint);
				// A stale authored-door route must never override a player who is already
				// visible in the same room.
				if(bCloseToPlayer&&ActiveGate.IsValid())ClearGateTraversal();
				if(ActiveGate.IsValid()&&UpdateGateTraversal(RouteWaypoint))return;
				if(bRouteBlocked&&UpdateGateTraversal(RouteWaypoint))return;

				// A detour must be serviced before the trail branch. Previously the trail
				// returned every tick, so a successfully selected detour was immediately
				// ignored and the zombie kept steering into the same corner.
				if(bUsingDetour)
				{
					const bool bDetourFinished=FVector::DistSquared2D(Zombie->GetActorLocation(),DetourLocation)<=FMath::Square(PatrolAcceptanceRadius)
						||GetMoveStatus()==EPathFollowingStatus::Idle||Now>=DetourExpireTime;
					if(!bDetourFinished)
					{
						TryJumpObstacle(DetourLocation);
						return;
					}
					bUsingDetour=false;
					NextPathRefreshTime=0.;
				}

				if(bFollowingPlayerTrail)
				{
					if(TryJumpObstacle(RouteWaypoint))return;
					// Closed doors and destructible partitions commonly make this trail
					// segment partial. The old early return prevented the normal blocking-
					// structure attack from ever running, leaving the horde at the wall.
					if(Now-LastChaseProgressTime>=StuckRecoveryDelay)
					{
						if(TryAttackBlockingStructure(RouteWaypoint))return;
						if(TryStartDetour(RouteWaypoint))return;
						// This breadcrumb has not produced movement. Reinitialize from the
						// newest reachable/visible point after the physical escape.
						PursuitTrailCursor=INDEX_NONE;
						if(StartCornerEscape(RouteWaypoint))return;
					}
					bool bCompleteTrailPath=false;
					if(UNavigationPath* TrailPath=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),Zombie->GetActorLocation(),RouteWaypoint,Zombie))
						bCompleteTrailPath=TrailPath->IsValid()&&!TrailPath->IsPartial();
					if(GetMoveStatus()==EPathFollowingStatus::Idle||Now>=NextPathRefreshTime)
					{
						const EPathFollowingRequestResult::Type TrailMove=bCompleteTrailPath?MoveToLocation(RouteWaypoint,95.f,true,true,true):EPathFollowingRequestResult::Failed;
						NextPathRefreshTime=Now+.4;
						if(TrailMove==EPathFollowingRequestResult::Failed)SteerToward(RouteWaypoint);
					}
					else if(!bCompleteTrailPath)SteerToward(RouteWaypoint);
					return;
				}
				// Dynamic invoker tiles need a short time to appear in a packaged game.
				// Continue physical steering during that interval instead of standing still.
				if(Now<ForceSteeringUntil)
				{
					if(TryJumpObstacle(Destination))return;
					SteerToward(Destination);
					return;
				}
				if(TryJumpObstacle(Destination))return;
				if(Now-LastChaseProgressTime>=StuckRecoveryDelay)
				{
					if(bRouteBlocked&&UpdateGateTraversal(RouteWaypoint))return;
					if(TryAttackBlockingStructure(Destination))return;
					if(TryStartDetour(Destination))return;
					if(StartCornerEscape(Destination))return;
					LastChaseProgressLocation=Zombie->GetActorLocation();
					LastChaseProgressTime=Now;
					StopMovement();
					ForceSteeringUntil=Now+StuckRecoveryDelay;
					SteerToward(Destination);
					return;
				}

				if(GetMoveStatus()==EPathFollowingStatus::Idle||Now>=NextPathRefreshTime)
				{
					const EPathFollowingRequestResult::Type MoveResult=bCanSeeTarget
						?MoveToActor(Target.Get(),Zombie->AttackRange*.75f,true,true,true)
						:MoveToLocation(Destination,PatrolAcceptanceRadius,true,true,true);
					NextPathRefreshTime=Now+PathRefreshInterval;
					if(MoveResult==EPathFollowingRequestResult::Failed)
					{
						if(bRouteBlocked&&UpdateGateTraversal(RouteWaypoint))return;
						if(TryAttackBlockingStructure(Destination))return;
						if(TryStartDetour(Destination))return;
						if(StartCornerEscape(Destination))return;
						ForceSteeringUntil=Now+StuckRecoveryDelay;
						SteerToward(Destination);
					}
				}
			}
			return;
		}
	}

	if(bSteeringPatrol)
	{
		if(FVector::DistSquared2D(Zombie->GetActorLocation(),PatrolLastProgressLocation)>FMath::Square(30.f))
		{
			PatrolLastProgressLocation=Zombie->GetActorLocation();
			PatrolLastProgressTime=Now;
		}
		if(Now-PatrolLastProgressTime>=PatrolStuckTimeout)
		{
			StopMovement();
			bSteeringPatrol=false;
			bPatrolMoveActive=false;
			NextPatrolTime=Now+.2f;
			return;
		}
		if(FVector::DistSquared2D(Zombie->GetActorLocation(),SteeringPatrolLocation)<=FMath::Square(PatrolAcceptanceRadius))
		{
			bSteeringPatrol=false;
			bPatrolMoveActive=false;
			NextPatrolTime=Now+FMath::FRandRange(PatrolWaitMin,PatrolWaitMax);
		}
		else SteerToward(SteeringPatrolLocation);
		return;
	}

	if(bPatrolMoveActive&&FVector::DistSquared2D(Zombie->GetActorLocation(),PatrolLastProgressLocation)>FMath::Square(30.f))
	{
		PatrolLastProgressLocation=Zombie->GetActorLocation();
		PatrolLastProgressTime=Now;
	}
	if(bPatrolMoveActive&&(GetMoveStatus()==EPathFollowingStatus::Idle||Now-PatrolLastProgressTime>=PatrolStuckTimeout))
	{
		StopMovement();
		bPatrolMoveActive=false;
		NextPatrolTime=Now+(Now-PatrolLastProgressTime>=PatrolStuckTimeout?.2f:FMath::FRandRange(PatrolWaitMin,PatrolWaitMax));
	}
	if(!bPatrolMoveActive&&Now>=NextPatrolTime)TryStartPatrol();
}
