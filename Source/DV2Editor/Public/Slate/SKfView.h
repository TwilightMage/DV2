#pragma once
#include "SNiView.h"

class SKfView : public SNiView
{
public:
	SLATE_BEGIN_ARGS(SKfView)
		{

		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset);

private:
	virtual TSharedPtr<SWidget> MakeViewportWidget(const TSharedPtr<FNiFile>& InFile) override;
};
