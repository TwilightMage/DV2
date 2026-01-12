#pragma once

struct FDV2AssetTreeEntry;

class FFileHandlerBase
{
public:
	virtual ~FFileHandlerBase() = default;
	
	virtual TSharedPtr<SWidget> CreateViewForFile(const TSharedPtr<FDV2AssetTreeEntry>& Asset) = 0;
	
#if WITH_EDITOR
	virtual void ConfigureMenu(const TSharedPtr<FDV2AssetTreeEntry>& Asset, FMenuBuilder& MenuBuilder) {}
#endif
};
