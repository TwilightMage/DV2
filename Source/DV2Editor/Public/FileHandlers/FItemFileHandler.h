#pragma once
#include "FFileHandlerBase.h"
#include "DV2Editor/Public/Slate/SItemView.h"

class FItemFileHandler : public FFileHandlerBase
{
public:
	virtual TSharedPtr<SWidget> CreateViewForFile(const TSharedPtr<FDV2AssetTreeEntry>& Asset) override { return SNew(SItemView, Asset); }
};
