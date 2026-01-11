#include "Slate/SKFTrack.h"

#include "NetImmerse.h"

void SKFTrack::Construct(const FArguments& InArgs, const TSharedPtr<FNiBlock>& InTrackBlock)
{
	TrackBlock = InTrackBlock;
	if (!TrackBlock->IsOfType("NiControllerSequence"))
	{
		ChildSlot
		[
			SNew(SBox)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Invalid Block Type"))
			]
		];
		return;
	}

	
}
