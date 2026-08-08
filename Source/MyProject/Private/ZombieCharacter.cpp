#include "ZombieCharacter.h"
#include "ZombieAIController.h"
#include "HealthArmorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
AZombieCharacter::AZombieCharacter(){bReplicates=true;AIControllerClass=AZombieAIController::StaticClass();AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;Health=CreateDefaultSubobject<UHealthArmorComponent>(TEXT("Health"));Health->MaxHealth=120.f;Health->OnDeath.AddDynamic(this,&AZombieCharacter::HandleDeath);GetCharacterMovement()->MaxWalkSpeed=350.f;GetCharacterMovement()->bOrientRotationToMovement=true;}
bool AZombieCharacter::TryAttack(AActor*T){if(!HasAuthority()||!T||FVector::DistSquared(T->GetActorLocation(),GetActorLocation())>FMath::Square(AttackRange))return false;const double N=GetWorld()->GetTimeSeconds();if(N-LastAttackTime<AttackCooldown)return false;LastAttackTime=N;if(UHealthArmorComponent*H=T->FindComponentByClass<UHealthArmorComponent>())H->ApplyDamage(AttackDamage,GetController(),this);return true;}
void AZombieCharacter::HandleDeath(){if(AAIController*C=Cast<AAIController>(GetController()))C->StopMovement();GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));GetMesh()->SetSimulatePhysics(true);SetLifeSpan(20.f);}
