#include "Slate/SItemView.h"

#include "Slate/SNifSceneViewport.h"

void SItemView::Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset)
{
	SNiView::Construct(SNiView::FArguments(), InAsset);
}

TSharedPtr<SWidget> SItemView::MakeViewportWidget(const TSharedPtr<FNiFile>& InFile)
{
	if (InFile->Blocks.Num() > 0 && InFile->Blocks[0]->IsOfType("CStreamableAssetData"))
	{
		uint32 RootBlockIndex = InFile->Blocks[0]->ReferenceAt("Root").Index;

		SAssignNew(NifSceneViewport, SNifSceneViewport)
		.OrbitingCamera(true);

		NifSceneViewport->GetNifViewportClient()->LoadNifFile(InFile, RootBlockIndex);

		return NifSceneViewport;
	}

	return nullptr;
}

void SItemView::OnSelectedBlockChanged(const TSharedPtr<FNiBlock>& Block)
{
	NifSceneViewport->GetNifViewportClient()->SetSelectedBlock(Block);
}
