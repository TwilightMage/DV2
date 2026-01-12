#pragma once
#include "FFileHandlerBase.h"

class FLuaFileHandler : public FFileHandlerBase
{
public:
	virtual TSharedPtr<SWidget> CreateViewForFile(const TSharedPtr<FDV2AssetTreeEntry>& Asset) override;
};
