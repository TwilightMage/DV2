#pragma once

class FNiBlock;

class SKFTrack : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKFTrack)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FNiBlock>& InTrackBlock);

private:
	TSharedPtr<FNiBlock> TrackBlock;
};
