#pragma once

#include "CoreMinimal.h"
#include "ZombieCharacter.h"
#include "WalkerZombieCharacter.generated.h"

/** Ordinary walker that uses the newly imported, mutually compatible mesh and animations. */
UCLASS(Blueprintable)
class MYPROJECT_API AWalkerZombieCharacter : public AZombieCharacter
{
	GENERATED_BODY()

public:
	AWalkerZombieCharacter();
};
