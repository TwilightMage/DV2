#pragma once
#include "DV2Ghost.h"
#include "IDetailCustomization.h"

class ADV2Ghost;
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
		if (ADV2Ghost* Ghost = Cast<ADV2Ghost>(Object))
		{
			Ghost->SetFile(NewFile);
		}
	}

	virtual void Revert(UObject* Object) override
	{
		if (ADV2Ghost* Ghost = Cast<ADV2Ghost>(Object))
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
	void Refresh();
	void SetFile(const FString& NewPath);

	ADV2Ghost* Target;
	TSharedPtr<SMenuAnchor> MenuAnchor;
	TSharedPtr<SEditableTextBox> PathDisplay;
};
