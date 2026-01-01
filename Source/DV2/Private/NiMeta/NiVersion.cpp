#include "NiMeta/NiVersion.h"

FNiVersion FNiVersion::Parse(const FString& str)
{
	FNiVersion result = {0, 0, 0, 0};

	const TCHAR* chr = *str;
	const TCHAR* end = *str + str.Len();

	const TCHAR* lexStart = chr;
	uint8* bitPtr = &result.v1;
	while (true)
	{
		if (!TChar<TCHAR>::IsDigit(*chr) || chr == end)
		{
			if (*chr != '.' && chr != end)
				return FNiVersion();

			FStringView view(lexStart, chr - lexStart);

			LexFromString(*bitPtr, view);

			bitPtr++;
			lexStart = chr + 1;
		}

		if (chr == end)
			break;

		chr++;
	}

	return result;
}

bool FNiVersion::operator==(const FNiVersion& rhs) const
{
	return v1 == rhs.v1 && v2 == rhs.v2 && v3 == rhs.v3 && v4 == rhs.v4;
}

bool FNiVersion::operator!=(const FNiVersion& rhs) const
{
	return !(*this == rhs);
}

bool FNiVersion::operator<(const FNiVersion& rhs) const
{
	if (v1 < rhs.v1)
		return true;
	if (v1 == rhs.v1)
	{
		if (v2 < rhs.v2)
			return true;
		if (v2 == rhs.v2)
		{
			if (v3 < rhs.v3)
				return true;
			if (v3 == rhs.v3)
			{
				return v4 < rhs.v4;
			}
		}
	}

	return false;
}

bool FNiVersion::operator<=(const FNiVersion& rhs) const
{
	if (v1 < rhs.v1)
		return true;
	if (v1 == rhs.v1)
	{
		if (v2 < rhs.v2)
			return true;
		if (v2 == rhs.v2)
		{
			if (v3 < rhs.v3)
				return true;
			if (v3 == rhs.v3)
			{
				return v4 <= rhs.v4;
			}
		}
	}

	return false;
}

bool FNiVersion::operator>(const FNiVersion& rhs) const
{
	if (v1 > rhs.v1)
		return true;
	if (v1 == rhs.v1)
	{
		if (v2 > rhs.v2)
			return true;
		if (v2 == rhs.v2)
		{
			if (v3 > rhs.v3)
				return true;
			if (v3 == rhs.v3)
			{
				return v4 > rhs.v4;
			}
		}
	}

	return false;
}

bool FNiVersion::operator>=(const FNiVersion& rhs) const
{
	if (v1 > rhs.v1)
		return true;
	if (v1 == rhs.v1)
	{
		if (v2 > rhs.v2)
			return true;
		if (v2 == rhs.v2)
		{
			if (v3 > rhs.v3)
				return true;
			if (v3 == rhs.v3)
			{
				return v4 >= rhs.v4;
			}
		}
	}

	return false;
}
