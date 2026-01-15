#pragma once
#include "NetImmerse.h"
#include "SDV2AssetViewBase.h"
#include "SNiBlockInspector.h"

class SNiOutliner;
class SNiBlockInspector;
class FNiBlock;
struct FNiFile;

class SNiView : public SDV2AssetViewBase
{
public:
	TSharedPtr<FNiFile> GetFile() const { return File; }
	
protected:
	virtual TSharedPtr<SWidget> MakeViewportWidget(const TSharedPtr<FNiFile>& File) = 0;
	virtual void OnSelectedBlockChanged(const TSharedPtr<FNiBlock>& Block) {}

private:
	virtual void OnConstructView(const TSharedPtr<FDV2AssetTreeEntry>& InAsset) override;
	TSharedRef<SWidget> MakeOutlinerWidget();
	TSharedRef<SWidget> MakeInspectorWidget();

	virtual FNiMask* GetMask() { return nullptr; }
	virtual void OnMaskEdited() {}

	TSharedPtr<FNiFile> File;
	TSharedPtr<SNiOutliner> NiOutliner;
	TSharedPtr<FNiBlock> SelectedBlock;
	TSharedPtr<SNiBlockInspector> NiBlockInspector;

	float InspectorWidth = 350;
	float InspectorHeight = 700;
};
