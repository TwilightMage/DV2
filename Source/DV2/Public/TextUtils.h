#pragma once

#define FORMAT_TEXT(Namespace, Pattern, ...) FText::Format(NSLOCTEXT(Namespace, Pattern, Pattern), __VA_ARGS__)
#define FORMAT_TEXT_KEY(Namespace, Key, Pattern, ...) FText::Format(NSLOCTEXT(Namespace, Key, Pattern), __VA_ARGS__)
#define FORMAT_TEXT_INSTRUCT(Pattern, ...) FText::Format(FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT(Pattern), *StaticStruct()->GetName(), TEXT(Pattern)), __VA_ARGS__)
#define FORMAT_TEXT_INSTRUCT_KEY(Key, Pattern, ...) FText::Format(FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT(Pattern), *StaticStruct()->GetName(), TEXT(Key)), __VA_ARGS__)
#define FORMAT_TEXT_INCLASS(Pattern, ...) FText::Format(FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT(Pattern), *StaticClass()->GetName(), TEXT(Pattern)), __VA_ARGS__)
#define FORMAT_TEXT_INCLASS_KEY(Key, Pattern, ...) FText::Format(FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT(Pattern), *StaticClass()->GetName(), TEXT(Key)), __VA_ARGS__)
#define MAKE_TEXT(Namespace, Text) FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT(Text), TEXT(Namespace), TEXT(Text))
#define MAKE_TEXT_INSTRUCT(Text) FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT(Text), *StaticStruct()->GetName(), TEXT(Text))
#define MAKE_TEXT_INSTRUCT_KEY(Key, Text) FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT(Text), *StaticStruct()->GetName(), TEXT(Key))
#define MAKE_TEXT_INCLASS(Text) FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT(Text), *StaticClass()->GetName(), TEXT(Text))
#define MAKE_TEXT_INCLASS_KEY(Key, Text) FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TEXT(Text), *StaticClass()->GetName(), TEXT(Key))