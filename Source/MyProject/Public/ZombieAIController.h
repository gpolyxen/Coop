#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "ZombieAIController.generated.h"
class UAIPerceptionComponent;class UAISenseConfig_Sight;class UAISenseConfig_Hearing;
UCLASS()
class MYPROJECT_API AZombieAIController:public AAIController
{
	GENERATED_BODY()
public:
	AZombieAIController();
	virtual void Tick(float DeltaSeconds)override;
	void AlertToActor(AActor* Actor);
protected:virtual void OnPossess(APawn* InPawn)override;
	UPROPERTY(VisibleAnywhere)UAIPerceptionComponent* Senses;UPROPERTY()UAISenseConfig_Sight* Sight;UPROPERTY()UAISenseConfig_Hearing* Hearing;
	UFUNCTION()void OnTargetPerception(AActor* Actor,FAIStimulus Stimulus);
	void TryStartPatrol();
	void TryAcquireVisibleTarget();
	void SteerToward(const FVector& Destination);
	bool TryJumpObstacle(const FVector& Destination);
	bool TryStartDetour(const FVector& Destination);
	bool TryAttackBlockingStructure(const FVector& Destination);
	bool UpdateStairTraversal(const FVector& Destination);
	void ClearStairTraversal();
	bool UpdateGateTraversal(const FVector& Destination);
	FVector GetVerticalRouteWaypoint(const FVector& Destination)const;
	bool IsAuthoredRouteBlocked(const FVector& Waypoint)const;
	void ClearGateTraversal();
	void AlertNearbyHorde(AActor* Actor);
	void ResetNavigationRecovery();
	TWeakObjectPtr<AActor> Target;
	FVector LastKnownLocation;
	// Multi-storey pursuit takes longer than the old eight-second perception
	// memory.  Keep a known player long enough to traverse several stair modules.
	// Once a wave has found a living player it must keep hunting while the player
	// is hiding on an upper floor.  A short perception timeout made the horde drop
	// aggro half way through a multi-storey route and start wandering downstairs.
	UPROPERTY(EditDefaultsOnly,Category="AI")float ForgetAfter=300.f;
	// At arm's length a player is noticed even when he approaches from behind.
	// An obstacle must still be absent, so this does not provide vision through walls.
	UPROPERTY(EditDefaultsOnly,Category="AI",meta=(ClampMin="100"))float ProximityAwarenessRadius=350.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolRadius=1500.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolAcceptanceRadius=80.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolWaitMin=1.f;
	UPROPERTY(EditDefaultsOnly,Category="AI")float PatrolWaitMax=4.f;
	UPROPERTY(EditDefaultsOnly,Category="AI",meta=(ClampMin="500"))float HordeAlertRadius=20000.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Navigation",meta=(ClampMin="0.1"))float PathRefreshInterval=.65f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Navigation",meta=(ClampMin="0.5"))float StuckRecoveryDelay=1.25f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Navigation",meta=(ClampMin="100"))float DetourSearchRadius=450.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Navigation",meta=(ClampMin="1.0"))float DetourTimeout=3.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="60"))float JumpProbeDistance=145.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="1"))float MinimumJumpObstacleHeight=35.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="20"))float MaximumJumpObstacleHeight=105.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="100"))float JumpForwardVelocity=430.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="0.1"))float JumpCooldown=1.2f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="0"))float JumpClearance=12.f;
	UPROPERTY(EditDefaultsOnly,Category="AI|Jump",meta=(ClampMin="0"))float MaximumLandingDrop=90.f;
	double LastStimulus=-1000.;
	double NextPatrolTime=0.;
	bool bPatrolMoveActive=false;
	bool bSteeringPatrol=false;
	FVector SteeringPatrolLocation=FVector::ZeroVector;
	FVector PatrolLastProgressLocation=FVector::ZeroVector;
	double PatrolLastProgressTime=0.;
	UPROPERTY(EditDefaultsOnly,Category="AI|Patrol",meta=(ClampMin="0.5"))float PatrolStuckTimeout=2.5f;
	FVector LastChaseProgressLocation=FVector::ZeroVector;
	double LastChaseProgressTime=0.;
	double NextPathRefreshTime=0.;
	int32 PursuitTrailCursor=INDEX_NONE;
	double LastJumpTime=-1000.;
	double DetourExpireTime=0.;
	double ForceSteeringUntil=0.;
	bool bUsingDetour=false;
	FVector DetourLocation=FVector::ZeroVector;
	FVector JumpTravelDirection=FVector::ZeroVector;
	TWeakObjectPtr<class AWoodStairs> ActiveStairs;
	FVector StairEntry=FVector::ZeroVector;
	FVector StairExit=FVector::ZeroVector;
	bool bTraversingStairs=false;
	double NextStairMoveRefresh=0.;
	double StairApproachExpireTime=0.;
	bool bStairEntryHasNavPath=false;
	FVector StairLastProgressLocation=FVector::ZeroVector;
	double StairLastProgressTime=0.;
	double LastStairRecoveryTime=-1000.;
	TWeakObjectPtr<class AWoodGate> ActiveGate;
	TWeakObjectPtr<class AWoodGate> LastTraversedGate;
	FVector GateEntry=FVector::ZeroVector;
	FVector GateExit=FVector::ZeroVector;
	bool bTraversingGate=false;
	double NextGateMoveRefresh=0.;
	double GateApproachExpireTime=0.;
	double GateReuseAllowedTime=0.;
	bool bGateEntryHasNavPath=false;
	FVector GateLastProgressLocation=FVector::ZeroVector;
	double GateLastProgressTime=0.;
	bool bLoggedInitialSightScan=false;
	bool bDiagnosticLogging=false;
	double NextDiagnosticLogTime=0.;
};
