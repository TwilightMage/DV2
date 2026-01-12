#pragma once

struct FDV2AssetTreeEntry;

class SDV2AssetViewBase : public SCompoundWidget
{
public:
	const TSharedPtr<FDV2AssetTreeEntry>& GetAsset() const { return Asset; }

protected:
	void ConstructAssetView(const TSharedPtr<FDV2AssetTreeEntry>& InAsset);
	
	void FailConstruct(const FString& Message);
	bool ReadAsset(TArray<uint8>& OutBytes) const;

	virtual void OnConstructView(const TSharedPtr<FDV2AssetTreeEntry>& InAsset) = 0;

private:
	TSharedPtr<FDV2AssetTreeEntry> Asset;
};
