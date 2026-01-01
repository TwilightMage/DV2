#pragma once
#include "DV2.h"
#include "NiMeta.h"

namespace NiMeta
{
	struct structType;
	struct basicType;

	template <typename TType, typename... T>
	void Patch(TMap<FString, TSharedPtr<TType>> map, const TFunction<void(TType& entry)>& handler, T&&... names)
	{
		auto Process = [&](const FString& name)
		{
			if (auto entry = map.FindRef(name); entry.IsValid())
			{
				handler(*entry);
				UE_LOG(LogDV2, Display, TEXT("Patched entry %s"), *name);
			}
			else
			{
				UE_LOG(LogDV2, Warning, TEXT("Failed to patch entry %s, not found"), *name);
			}
		};

		(Process(FString(std::forward<T>(names))), ...);
	}

	template <typename... T>
	void PatchBasic(const TFunction<void(basicType& entry)>& handler, T&&... names)
	{
		Patch<basicType>(BasicMap(), handler, std::forward<T>(names)...);
	}

	template <typename... T>
	void PatchStruct(const TFunction<void(structType& entry)>& handler, T&&... names)
	{
		Patch<structType>(StructMap(), handler, std::forward<T>(names)...);
	}

	template <typename... T>
	void PatchNiObject(const TFunction<void(niobject& entry)>& handler, T&&... names)
	{
		Patch<niobject>(NiObjectMap(), handler, std::forward<T>(names)...);
	}

	void PatchNifMeta();
}
