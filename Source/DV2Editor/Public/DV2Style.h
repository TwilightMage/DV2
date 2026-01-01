#pragma once

namespace NiMeta
{
	struct niobject;
}

class FDV2Style
{
public:
	static void Initialize();
	static void Shutdown();
	static const ISlateStyle& Get();
	static FName GetStyleSetName();

	static bool FindNifBlockIcon(const TSharedPtr<NiMeta::niobject>& BlockType, const FSlateBrush*& OutIconBrush);

private:
	inline static TSharedPtr<FSlateStyleSet> StyleInstance = nullptr;
};
