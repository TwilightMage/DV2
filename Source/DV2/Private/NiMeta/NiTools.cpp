#include "NiMeta/NiTools.h"

#include "NetImmerse.h"

namespace NiTools
{
	FString ReadString(const FNiFile& File, const FNiField& Field, uint32 Index)
	{
		if (Index >= Field.Size)
			return "";
		
		const auto& Item = *Field.GroupAt(Index);
		uint32 StringIndex = Item["Index"].SingleNumber<uint32>();

		if (StringIndex == (uint32)-1)
			return "";

		if (StringIndex >= (uint32)File.Strings.Num())
			return "";

		return File.Strings[StringIndex];
	}

	FVector ReadVector3(const FNiField& Field, uint32 Index)
	{
		if (Index >= Field.Size)
			return FVector::ZeroVector;

		const auto& Item = *Field.GroupAt(Index);
		auto X = Item["x"].SingleNumber<float>();
		auto Y = Item["y"].SingleNumber<float>();
		auto Z = Item["z"].SingleNumber<float>();

		return FVector(X, Y, Z);
	}

	FColor ReadColorFloat(const FNiField& Field, uint32 Index)
	{
		if (Index >= Field.Size)
			return FColor::White;

		const auto& Item = *Field.GroupAt(Index);
		float r = Item["r"].SingleNumber<float>();
		float g = Item["g"].SingleNumber<float>();
		float b = Item["b"].SingleNumber<float>();

		float a;
		if (auto fld = Item.FindField("a"))
			a = fld->SingleNumber<float>();
		else
			a = 1.0f;

		return FLinearColor(r, g, b, a).ToFColor(true);
	}

	FColor ReadColorByte(const FNiField& Field, uint32 Index)
	{
		if (Index >= Field.Size)
			return FColor::White;

		const auto& Item = *Field.GroupAt(Index);
		uint8 r = Item["r"].SingleNumber<uint8>();
		uint8 g = Item["g"].SingleNumber<uint8>();
		uint8 b = Item["b"].SingleNumber<uint8>();

		uint8 a;
		if (auto fld = Item.FindField("a"))
			a = fld->SingleNumber<uint8>();
		else
			a = 255;

		return FColor(r, g, b, a);
	}

	FVector2D ReadVector2(const FNiField& Field, uint32 Index)
	{
		if (Index >= Field.Size)
			return FVector2D::ZeroVector;

		const auto& Item = *Field.GroupAt(Index);
		auto X = Item["x"].SingleNumber<float>();
		auto Y = Item["y"].SingleNumber<float>();

		return FVector2D(X, Y);
	}

	FVector2D ReadUV(const FNiField& Field, uint32 Index)
	{
		if (Index >= Field.Size)
			return FVector2D::ZeroVector;

		const auto& Item = *Field.GroupAt(Index);
		auto U = Item["u"].SingleNumber<float>();
		auto V = Item["v"].SingleNumber<float>();

		return FVector2D(U, V);
	}

	FRotator ReadRotator(const FNiField& Field, uint32 Index)
	{
		if (Index >= Field.Size)
			return FRotator::ZeroRotator;

		const auto& Item = *Field.GroupAt(Index);

		float m11 = Item["m11"].SingleNumber<float>();
		float m21 = Item["m21"].SingleNumber<float>();
		float m31 = Item["m31"].SingleNumber<float>();
		float m12 = Item["m12"].SingleNumber<float>();
		float m22 = Item["m22"].SingleNumber<float>();
		float m32 = Item["m32"].SingleNumber<float>();
		float m13 = Item["m13"].SingleNumber<float>();
		float m23 = Item["m23"].SingleNumber<float>();
		float m33 = Item["m33"].SingleNumber<float>();

		FMatrix matrix = FMatrix(
			FVector(m11, m12, m13),
			FVector(m21, m22, m23),
			FVector(m31, m32, m33),
			FVector::ZeroVector
			);

		return matrix.Rotator();
	}
}
