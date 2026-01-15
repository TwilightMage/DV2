#pragma once

struct FDV2AssetTreeEntry;

class FAssetContextMenuGenerator : public TSharedFromThis<FAssetContextMenuGenerator>
{
public:
	virtual void GenerateMenu(FMenuBuilder& MenuBuilder, const TSharedPtr<FDV2AssetTreeEntry>& Asset) = 0;
};

class FDefaultAssetContextMenuGenerator : public FAssetContextMenuGenerator
{
public:
	virtual void GenerateMenu(FMenuBuilder& MenuBuilder, const TSharedPtr<FDV2AssetTreeEntry>& Asset) override;

private:
	void ExportXml(TSharedPtr<FDV2AssetTreeEntry> Asset) const;
	void PreviewVegetationMap(TSharedPtr<FDV2AssetTreeEntry> Asset) const;
};
