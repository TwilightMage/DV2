#pragma once

#include "DV2Importer.generated.h"

UCLASS(Abstract)
class UDV2Importer : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, DisplayName="Unpack DV2")
	static bool Unpack(const FString& dv2, const FString& dir);

	UFUNCTION(BlueprintCallable, DisplayName="Unpack DV2 With Wizard")
	static bool UnpackWithWizard();
};