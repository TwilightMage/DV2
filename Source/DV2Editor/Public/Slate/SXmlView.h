#pragma once
#include "SDV2AssetViewBase.h"

class FCStreamableNode;
struct FNiFile;
class FNiBlock_CStreamableNode;

class SXmlView : public SDV2AssetViewBase
{
public:
	SLATE_BEGIN_ARGS(SXmlView)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FDV2AssetTreeEntry>& InAsset);

private:
	virtual void OnConstructView(const TSharedPtr<FDV2AssetTreeEntry>& InAsset) override;

	TSharedPtr<FNiFile> File;
	TSharedPtr<FNiBlock_CStreamableNode> Block;
};

class SXmlViewTableRow : public SMultiColumnTableRow<TSharedPtr<FCStreamableNode>>
{
public:
	SLATE_BEGIN_ARGS(SXmlViewTableRow)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& Owner, const TSharedPtr<FCStreamableNode>& InTarget);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	TSharedPtr<FCStreamableNode> Target;
};
