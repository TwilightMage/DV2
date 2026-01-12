#pragma once
#include "FFileHandlerBase.h"
#include "DV2Editor/Public/Slate/SXmlView.h"

class FXmlFileHandler : public FFileHandlerBase
{
public:
	virtual TSharedPtr<SWidget> CreateViewForFile(const TSharedPtr<FDV2AssetTreeEntry>& Asset) override { return SNew(SXmlView, Asset); }

#if WITH_EDITOR
	virtual void ConfigureMenu(const TSharedPtr<FDV2AssetTreeEntry>& Asset, FMenuBuilder& MenuBuilder) override;
#endif

private:
#if WITH_EDITOR
	void ExportXml(TSharedPtr<FDV2AssetTreeEntry> Asset) const;
#endif
};
