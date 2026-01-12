#pragma once

class SDV2Explorer;
class SNifSceneViewport;
class SNiBlockInspector;
struct FNiFile;
class FNiBlock;

#include "DV2AssetTree.h"

class FDV2Browser : public TSharedFromThis<FDV2Browser>
{
public:
	TSharedRef<SDockTab> Open(const FSpawnTabArgs& SpawnTabArgs);
	void Init();
	
	static FDV2Browser& Get();
	
	inline static FName TabName = "DV2Browser";
	inline static FName MenuName = "DV2Browser.MainMenu";

private:
	void SelectAsset(const TSharedPtr<FDV2AssetTreeEntry>& Entry, bool bSkipExplorer);
	TSharedPtr<SWidget> GetViewForAsset(const TSharedPtr<FDV2AssetTreeEntry>& asset) const;
	static TSharedPtr<SWidget> GetViewForAssetUnknown(const TSharedPtr<FDV2AssetTreeEntry>& asset);
	static TSharedPtr<SWidget> GetMessageViewForAsset(const FString& message);

	TSharedPtr<SDV2Explorer> AssetExplorer;
	TSharedPtr<SBox> AssetViewPanel;

	TSharedPtr<FTabManager> TabManager;
};
