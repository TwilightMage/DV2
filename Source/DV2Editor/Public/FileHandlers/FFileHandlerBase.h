#pragma once

struct FDV2AssetTreeEntry;

class FFileHandlerBase : public TSharedFromThis<FFileHandlerBase>
{
public:
	virtual TSharedPtr<SWidget> CreateViewForFile(const TSharedPtr<FDV2AssetTreeEntry>& Asset) = 0;
};
