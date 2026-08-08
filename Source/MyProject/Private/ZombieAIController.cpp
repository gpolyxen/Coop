#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "HealthArmorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "NavigationSystem.h"
AZombieAIController::AZombieAIController(){PrimaryActorTick.bCanEverTick=true;Senses=CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));SetPerceptionComponent(*Senses);Sight=CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));Sight->SightRadius=2200;Sight->LoseSightRadius=2800;Sight->PeripheralVisionAngleDegrees=75;Sight->SetMaxAge(ForgetAfter);Sight->DetectionByAffiliation.bDetectEnemies=true;Sight->DetectionByAffiliation.bDetectFriendlies=true;Hearing=CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));Hearing->HearingRange=4000;Hearing->SetMaxAge(ForgetAfter);Hearing->DetectionByAffiliation.bDetectEnemies=true;Hearing->DetectionByAffiliation.bDetectFriendlies=true;Senses->ConfigureSense(*Sight);Senses->ConfigureSense(*Hearing);Senses->SetDominantSense(Sight->GetSenseImplementation());Senses->OnTargetPerceptionUpdated.AddDynamic(this,&AZombieAIController::OnTargetPerception);}
void AZombieAIController::OnPossess(APawn*P){Super::OnPossess(P);}
void AZombieAIController::OnTargetPerception(AActor*A,FAIStimulus S){if(!A||A==GetPawn())return;if(S.WasSuccessfullySensed()){Target=A;LastKnownLocation=S.StimulusLocation.IsNearlyZero()?A->GetActorLocation():S.StimulusLocation;LastStimulus=GetWorld()->GetTimeSeconds();}}
void AZombieAIController::Tick(float D){Super::Tick(D);AZombieCharacter*Z=Cast<AZombieCharacter>(GetPawn());if(!Z)return;if(Target.IsValid()){if(UHealthArmorComponent*H=Target->FindComponentByClass<UHealthArmorComponent>())if(H->IsDead())Target=nullptr;}if(Target.IsValid()){LastKnownLocation=Target->GetActorLocation();const float Dist=FVector::Dist(Z->GetActorLocation(),LastKnownLocation);if(Dist<=Z->AttackRange){StopMovement();Z->TryAttack(Target.Get());}else MoveToActor(Target.Get(),Z->AttackRange*.75f,true,true,true);}else if(GetWorld()->GetTimeSeconds()-LastStimulus<ForgetAfter)MoveToLocation(LastKnownLocation,80.f,true,true,true);else StopMovement();}
