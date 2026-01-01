#pragma once

struct DV2_API FNiVersion
{
	uint8 v1 = 255;
	uint8 v2 = 255;
	uint8 v3 = 255;
	uint8 v4 = 255;

	static FNiVersion Parse(const FString& str);

	bool operator==(const FNiVersion& rhs) const;
	bool operator!=(const FNiVersion& rhs) const;
	bool operator<(const FNiVersion& rhs) const;
	bool operator<=(const FNiVersion& rhs) const;
	bool operator>(const FNiVersion& rhs) const;
	bool operator>=(const FNiVersion& rhs) const;

	bool IsValid() const { return v1 != 255 && v2 != 255 && v3 != 255 && v4 != 255; }
};