#pragma once
#include "NetImmerse.h"
#include "SDV2AssetViewBase.h"
#include "SNiBlockInspector.h"

class SNiBlockInspector;
class FNiBlock;
struct FNiFile;

class SNiView : public SDV2AssetViewBase
{
public:
	SLATE_BEGIN_ARGS(SNiView)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset);

protected:
	virtual TSharedPtr<SWidget> MakeViewportWidget(const TSharedPtr<FNiFile>& File) = 0;
	virtual void OnSelectedBlockChanged(const TSharedPtr<FNiBlock>& Block) {}
	
private:
	struct FBlockWrapper
	{
		TSharedPtr<FNiBlock> Block;
		TArray<TSharedPtr<FBlockWrapper>> CachedChildren;

		void GetChildren(TArray<TSharedPtr<FBlockWrapper>>& OutChildren)
		{
			if (CachedChildren.IsEmpty())
			{
				OutChildren.Reserve(Block->Referenced.Num());
				CachedChildren.Reserve(Block->Referenced.Num());
				for (const auto& Child : Block->Referenced)
				{
					TSharedPtr<FBlockWrapper> WrappedChild = MakeShareable(new FBlockWrapper{
						.Block = Child
					});
					OutChildren.Add(WrappedChild);
					CachedChildren.Add(WrappedChild);
				}
			}
			else
			{
				OutChildren.Append(CachedChildren);
			}
		}
	};

	virtual void OnConstructView(const TSharedPtr<FDV2AssetTreeEntry>& InAsset) override;
	TSharedRef<SWidget> MakeOutlinerWidget();
	TSharedRef<SWidget> MakeInspectorWidget();

	void RefreshRootNifNodes() const;

	TSharedPtr<FNiFile> File;
	TSharedPtr<UE::Slate::Containers::TObservableArray<TSharedPtr<FBlockWrapper>>> RootNifNodes;
	TSharedPtr<FBlockWrapper> SelectedBlock;
	TSharedPtr<STreeView<TSharedPtr<FBlockWrapper>>> NiOutliner;
	TSharedPtr<SNiBlockInspector> NiBlockInspector;

	float InspectorWidth = 350;
	float InspectorHeight = 700;
};
