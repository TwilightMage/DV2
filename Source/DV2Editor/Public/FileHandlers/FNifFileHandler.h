#pragma once
#include "FFileHandlerBase.h"
#include "DV2Editor/Public/Slate/SNifView.h"

class FNifFileHandler : public FFileHandlerBase
{
public:
	virtual TSharedPtr<SWidget> CreateViewForFile(const TSharedPtr<FDV2AssetTreeEntry>& Asset) override { return SNew(SNifView, Asset); }
};
