#pragma once

struct FDV2AssetTreeEntry;

class SDV2AssetViewBase : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDV2AssetViewBase)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset);

	const TSharedPtr<FDV2AssetTreeEntry>& GetAsset() const { return Asset; }

protected:
	void FailConstruct(const FString& Message);
	bool ReadAsset(TArray<uint8>& OutBytes) const;

	virtual void OnConstructView(const TSharedPtr<FDV2AssetTreeEntry>& InAsset) = 0;

private:
	TSharedPtr<FDV2AssetTreeEntry> Asset;
};
