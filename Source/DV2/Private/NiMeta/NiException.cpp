#include "NiMeta/NiException.h"

#include "NiMeta/NiMeta.h"

NiException& NiException::AddSource(const FString& Name, const FString& File, int32 Line)
{
	Sources.Add(Name, {File, Line});
	return *this;
}

NiException& NiException::AddSource(const FString& Name, const TSharedPtr<NiMeta::metaObject>& MetaObject)
{
	Sources.Add(Name, {NiMeta::metaObject::FilePaths[MetaObject->SourceFilePathIndex], MetaObject->SourceLine});
	return *this;
}
