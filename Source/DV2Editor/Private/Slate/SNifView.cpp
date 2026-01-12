#include "Slate/SNifView.h"

#include "DV2AssetTree.h"
#include "NetImmerse.h"
#include "Slate/SNifSceneViewport.h"

void SNifView::Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset)
{
	ConstructAssetView(InAsset);
}

TSharedPtr<SWidget> SNifView::MakeViewportWidget(const TSharedPtr<FNiFile>& InFile)
{
	SAssignNew(NifSceneViewport, SNifSceneViewport);

	NifSceneViewport->GetNifViewportClient()->LoadNifFile(InFile);

	return NifSceneViewport;
}

void SNifView::OnSelectedBlockChanged(const TSharedPtr<FNiBlock>& Block)
{
	NifSceneViewport->GetNifViewportClient()->SetSelectedBlock(Block);
}
