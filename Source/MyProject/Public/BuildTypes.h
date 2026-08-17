#pragma once

#include "CoreMinimal.h"
#include "BuildTypes.generated.h"

UENUM(BlueprintType)
enum class EBuildPieceType : uint8
{
	None,
	Bed,
	WoodWall,
	WoodGate,
	WoodFloor,
	WoodStairs,
	WoodPillar,
	WoodChest,
	WallTorch
};
