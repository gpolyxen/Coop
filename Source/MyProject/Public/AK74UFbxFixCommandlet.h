#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "AK74UFbxFixCommandlet.generated.h"

UCLASS()
class MYPROJECT_API UAK74UFbxFixCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UAK74UFbxFixCommandlet();
	virtual int32 Main(const FString& Params)override;
};
