// Fill out your copyright notice in the Description page of Project Settings.


#include "DV2LandscapeBrush.h"

UDV2LandscapeBrush::UDV2LandscapeBrush()
{
}

/*
UTextureRenderTarget2D* UDV2LandscapeBrush::RenderLayer_Native(const FLandscapeBrushParameters& InParameters)
{
	const bool bIsHeightmap = InParameters.LayerType == ELandscapeToolTargetType::Heightmap;

	FBox2D SectionBox2D(FVector2D(InParameters.RenderAreaWorldTransform.GetLocation()) - InParameters.RenderAreaSize,
	                    FVector2D(InParameters.RenderAreaWorldTransform.GetLocation()) + InParameters.RenderAreaSize);

	TArray<ADV2Ghost*> AffectedActors;
	for (auto Actor : UsedActors)
	{
		if (!Actor)
			continue;
		
		auto ActorBox = Actor->GetComponentsBoundingBox(true, true);
		auto ActorBox2D = FBox2D(FVector2D(ActorBox.Min.X, ActorBox.Min.Y), FVector2D(ActorBox.Max.X, ActorBox.Max.Y));

		if (SectionBox2D.Intersect(ActorBox2D))
			AffectedActors.Add(Actor);
	}

	RenderMeshesToSection(InParameters.CombinedResult, InParameters.RenderAreaWorldTransform.GetLocation(), InParameters.RenderAreaSize, AffectedActors);

	return InParameters.CombinedResult;
}

FBox UDV2LandscapeBrush::GetComponentsBoundingBox(bool bNonColliding, bool bIncludeFromChildActors) const
{
	if (UsedActors.IsEmpty())
		return FBox(GetActorLocation(), GetActorLocation());
	
	FBox Box(UsedActors[0]->GetComponentsBoundingBox(bNonColliding, bIncludeFromChildActors));
	for (int32 i = 1; i < UsedActors.Num(); i++)
		Box += UsedActors[i]->GetComponentsBoundingBox(bNonColliding, bIncludeFromChildActors);
	
	return Box;
}

void UDV2LandscapeBrush::GetActorBounds(bool bOnlyCollidingComponents, FVector& Origin, FVector& BoxExtent, bool bIncludeFromChildActors) const
{
	if (UsedActors.IsEmpty())
	{
		Origin = GetActorLocation();
		BoxExtent = FVector(0.0f, 0.0f, 0.0f);
		return;
	}
	
	FBox Box(UsedActors[0]->GetComponentsBoundingBox(!bOnlyCollidingComponents, bIncludeFromChildActors));
	for (int32 i = 1; i < UsedActors.Num(); i++)
		Box += UsedActors[i]->GetComponentsBoundingBox(!bOnlyCollidingComponents, bIncludeFromChildActors);
	
	Origin = Box.GetCenter();
	BoxExtent = Box.GetExtent();
}

void UDV2LandscapeBrush::RenderMeshesToSection(UTextureRenderTarget2D* InCombinedResult, const FVector& SectionLocation, const FIntPoint& SectionSize, const TArray<ADV2Ghost*>& AffectedActors)
{
	if (!InCombinedResult || AffectedActors.Num() == 0)
		return;

	int32 RTSize = InCombinedResult->SizeX;
	if (!InternalCaptureTarget || InternalCaptureTarget->SizeX != RTSize)
	{
		InternalCaptureTarget = NewObject<UTextureRenderTarget2D>();
		InternalCaptureTarget->InitAutoFormat(RTSize, RTSize);
		CaptureComponent->TextureTarget = InCombinedResult;
	}

	CaptureComponent->SetWorldLocation(SectionLocation + FVector(0.0f, 0.0f, 10000.0f));

	CaptureComponent->OrthoWidth = SectionSize.X;

	CaptureComponent->ShowOnlyActors.Empty();
	for (auto Actor : AffectedActors)
		CaptureComponent->ShowOnlyActors.Add(Actor);
	CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	CaptureComponent->CaptureScene();

	//UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, );
}
*/