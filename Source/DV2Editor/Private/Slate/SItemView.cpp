#include "Slate/SItemView.h"

#include "Slate/SNifSceneViewport.h"

void SItemView::Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset)
{
	ConstructAssetView(InAsset);
}

TSharedPtr<SWidget> SItemView::MakeViewportWidget(const TSharedPtr<FNiFile>& InFile)
{
	if (InFile->Blocks.Num() > 0)
	{
		uint32 RootBlockIndex = InFile->Blocks[0]->ReferenceAt("Root").Index;

		SAssignNew(NifSceneViewport, SNifSceneViewport)
		.OrbitingCamera(true);

		NifSceneViewport->GetNifViewportClient()->LoadNifFile(InFile, RootBlockIndex, &Mask);

		return NifSceneViewport;
	}

	return nullptr;
}

void SItemView::OnSelectedBlockChanged(const TSharedPtr<FNiBlock>& Block)
{
	NifSceneViewport->GetNifViewportClient()->SetSelectedBlock(Block);
}

void SItemView::OnMaskEdited()
{
	if (GetFile()->Blocks.Num() > 0)
	{
		uint32 RootBlockIndex = GetFile()->Blocks[0]->ReferenceAt("Root").Index;

		NifSceneViewport->GetNifViewportClient()->LoadNifFile(GetFile(), RootBlockIndex, &Mask);
	}
}
