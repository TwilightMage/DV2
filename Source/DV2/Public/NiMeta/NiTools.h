#pragma once

struct FNiFile;
struct FNiField;

namespace NiTools
{
	DV2_API FString ReadString(const FNiFile& File, const FNiField& Field, uint32 Index = 0);
	DV2_API FVector ReadVector3(const FNiField& Field, uint32 Index = 0);
	DV2_API FColor ReadColorFloat(const FNiField& Field, uint32 Index = 0);
	DV2_API FVector2D ReadVector2(const FNiField& Field, uint32 Index = 0);
	DV2_API FVector2D ReadUV(const FNiField& Field, uint32 Index = 0);
	DV2_API FRotator ReadRotator(const FNiField& Field, uint32 Index = 0);
}
