#pragma once
#include "SNiView.h"

class SNifSceneViewport;

class SNifView : public SNiView
{
public:
	SLATE_BEGIN_ARGS(SNifView)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset);

private:
	virtual TSharedPtr<SWidget> MakeViewportWidget(const TSharedPtr<FNiFile>& InFile) override;
	virtual void OnSelectedBlockChanged(const TSharedPtr<FNiBlock>& Block) override;

	TSharedPtr<SNifSceneViewport> NifSceneViewport;
};
