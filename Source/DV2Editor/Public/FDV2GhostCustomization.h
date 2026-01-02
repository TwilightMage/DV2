#pragma once
#include "DV2Ghost.h"
#include "IDetailCustomization.h"

class ADV2GhostActor;
struct FDV2AssetTreeEntry;
struct FNiFile;

class FDV2GhostSetFile : public FCommandChange
{
public:
	FDV2GhostSetFile(const FString& InOldFile, const FString& InNewFile)
		: OldFile(InOldFile), NewFile(InNewFile)
	{
	}

	virtual void Apply(UObject* Object) override
	{
		if (UDV2GhostComponent* Ghost = Cast<UDV2GhostComponent>(Object))
		{
			Ghost->SetFile(NewFile);
		}
	}

	virtual void Revert(UObject* Object) override
	{
		if (UDV2GhostComponent* Ghost = Cast<UDV2GhostComponent>(Object))
		{
			Ghost->SetFile(OldFile);
		}
	}

	virtual FString ToString() const override { return TEXT("Custom Manual Change"); }

private:
	FString OldFile;
	FString NewFile;
};

class FDV2GhostCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FDV2GhostCustomization);
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	void SetFile(const FString& NewPath);
	FText GetNiPathTitle() const;

	UDV2GhostComponent* Target;
};
