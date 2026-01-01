#include "Slate/SKfView.h"

void SKfView::Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset)
{
	SNiView::Construct(SNiView::FArguments(), InAsset);
}

TSharedPtr<SWidget> SKfView::MakeViewportWidget(const TSharedPtr<FNiFile>& InFile)
{
	return SNullWidget::NullWidget;
}
