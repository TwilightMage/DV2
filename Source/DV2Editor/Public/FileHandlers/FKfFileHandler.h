#pragma once
#include "FFileHandlerBase.h"
#include "DV2Editor/Public/Slate/SKfView.h"

class FKfFileHandler : public FFileHandlerBase
{
public:
	virtual TSharedPtr<SWidget> CreateViewForFile(const TSharedPtr<FDV2AssetTreeEntry>& Asset) override { return SNew(SKfView, Asset); }
};
