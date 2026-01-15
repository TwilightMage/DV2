#pragma once
#include "SNiView.h"

class SNifSceneViewport;

class SItemView : public SNiView
{
public:
	SLATE_BEGIN_ARGS(SItemView)
	{
	}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset);

private:
	virtual TSharedPtr<SWidget> MakeViewportWidget(const TSharedPtr<FNiFile>& InFile) override;
	virtual void OnSelectedBlockChanged(const TSharedPtr<FNiBlock>& Block) override;

	virtual FNiMask* GetMask() override { return &Mask; }
	virtual void OnMaskEdited() override;
	
	FNiMask Mask;

	TSharedPtr<SNifSceneViewport> NifSceneViewport;
};
