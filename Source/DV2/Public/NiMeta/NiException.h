#pragma once

class NiException : public std::exception
{
public:
	NiException() = default;

	NiException(const TCHAR* message)
		: std::exception(TCHAR_TO_UTF8(message))
	{
	}

	NiException(const FString& message)
		: NiException(*message)
	{
	}

	template <typename... TArgs>
	NiException(UE::Core::TCheckedFormatString<FString::FmtCharType, TArgs...> fmt, TArgs... args)
		: NiException(FString::Printf(fmt, args...))
	{
	}

	const TCHAR* WhatTChar() const
	{
		return UTF8_TO_TCHAR(what());
	}
};