#pragma once

namespace NiMeta
{
	struct metaObject;
}

class NiException : public std::exception
{
public:
	struct SourcePosition
	{
		const char* File;
		int32 Line;
	};
	
	NiException() = default;

	NiException(const TCHAR* Message)
		: std::exception(TCHAR_TO_UTF8(Message))
	{
	}

	NiException( const FString& Message)
		: NiException(*Message)
	{
	}

	template <typename... TArgs>
	NiException(UE::Core::TCheckedFormatString<FString::FmtCharType, TArgs...> Fmt, TArgs... Args)
		: NiException(FString::Printf(Fmt, Args...))
	{
	}

	NiException(const NiException& InInnerException, const TCHAR* OuterMessage)
		: std::exception(TCHAR_TO_UTF8(OuterMessage))
		, InnerException(MakeShared<NiException>(InInnerException))
	{
	}

	NiException(const NiException& InInnerException, const FString& OuterMessage)
		: NiException(InInnerException, *OuterMessage)
	{
	}

	NiException& AddSource(const FString& Name, const FString& File, int32 Line);
	NiException& AddSource(const FString& Name, const TSharedPtr<NiMeta::metaObject>& MetaObject);

	FString GetWhatString() const
	{
		return what();
	}

	FString GetWhatFormatted() const
	{
		if (InnerException.IsValid())
			return GetWhatString() + " -> " + InnerException->GetWhatFormatted();
		else
			return GetWhatString();
	}

	void GetSources(TMap<FString, TTuple<FString, int32>>& OutSources) const
	{
		OutSources.Append(Sources);

		if (InnerException.IsValid())
			InnerException->GetSources(OutSources);
	}

	TMap<FString, TTuple<FString, int32>> Sources;
	TSharedPtr<NiException> InnerException;
};

#define MakeNiException(Msg) NiException(Msg).AddSource("Source", __FILE__, __LINE__)
#define MakeNiExceptionA(Msg, ...) NiException(TEXT(Msg), __VA_ARGS__).AddSource("Source", __FILE__, __LINE__)