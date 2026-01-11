#pragma once

class FXmlParser
{
public:
	virtual void OnComment(const FString& Comment, uint32 Line) = 0;
	virtual void OnTagOpen(const FString& TagName, uint32 Line) = 0;
	virtual void OnTagClose() = 0;
	virtual void OnAttribute(const FString& name, const FString& Value) = 0;
	virtual void OnText(const FString& Text) = 0;

	bool Parse(const TCHAR* FilePath, FString& OutError, uint32& OutErrorLine)
	{
		if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(FilePath))
		{
			OutError = "Invalid file path";
			OutErrorLine = (uint32)-1;
			return false;
		}

		FString Xml;
		if (!FFileHelper::LoadFileToString(Xml, FilePath))
		{
			OutError = "Failed to read file";
			OutErrorLine = (uint32)-1;
			return false;
		}

		return Parse(Xml, OutError, OutErrorLine);
	}

	bool Parse(const FString& Xml, FString& OutError, uint32& OutErrorLine)
	{
		if (Xml.IsEmpty())
			return true;

		enum class EState
		{
			Space,
			Tag,
			TagClose,
			Comment,
			Text,
		};

		enum class ESubState
		{
			Space,
			TagName,
			TagExit,
			AttributeName,
			AttributeGlue,
			AttributeValue,
		};

		EState State = EState::Space;
		ESubState SubState = ESubState::Space;

		const TCHAR* Chr = *Xml;
		const TCHAR* End = *Xml + Xml.Len();
		const TCHAR* ContextPos = nullptr;
		uint32 StateStartLineNumber = 1;
		uint32 LineNumber = 1;

		FString StoredAttributeName;

		while (Chr < End)
		{
			if (IsLineBreak(*Chr))
				LineNumber++;

			if (State == EState::Space)
			{
				if (*Chr == '<')
				{
					State = EState::Tag;
					SubState = ESubState::TagName;
					StateStartLineNumber = LineNumber;
					ContextPos = Chr;
				}
				else if (!IsSpace(*Chr))
				{
					State = EState::Text;
					ContextPos = Chr;
				}
			}
			else if (State == EState::Tag)
			{
				if (SubState == ESubState::Space)
				{
					if (*Chr == '/')
					{
						SubState = ESubState::TagExit;
					}
					else if (*Chr == '>')
					{
						State = EState::Space;
					}
					else if (!IsSpace(*Chr))
					{
						SubState = ESubState::AttributeName;
						ContextPos = Chr;
					}
				}
				else if (SubState == ESubState::TagName)
				{
					if (*Chr == '-' && *(Chr - 1) == '-' && *(Chr - 2) == '!')
					{
						State = EState::Comment;
					}
					else if (IsSpace(*Chr))
					{
						OnTagOpen(FString(Chr - ContextPos - 1, ContextPos + 1), StateStartLineNumber);
						SubState = ESubState::Space;
					}
					else if (*Chr == '>')
					{
						OnTagOpen(FString(Chr - ContextPos - 1, ContextPos + 1), StateStartLineNumber);
						State = EState::Space;
					}
					else if (*Chr == '/')
					{
						if (Chr == ContextPos + 1)
						{
							State = EState::TagClose;
						}
						else
						{
							OnTagOpen(FString(Chr - ContextPos - 1, ContextPos + 1), StateStartLineNumber);
							SubState = ESubState::TagExit;
						}
					}
				}
				else if (SubState == ESubState::AttributeName)
				{
					if (IsSpace(*Chr) || *Chr == '=')
					{
						SubState = ESubState::AttributeGlue;
						StoredAttributeName = FString(Chr - ContextPos, ContextPos);
					}
				
				}
				else if (SubState == ESubState::AttributeGlue)
				{
					if (*Chr == '"')
					{
						SubState = ESubState::AttributeValue;
						ContextPos = Chr;
					}
				}
				else if (SubState == ESubState::AttributeValue)
				{
					if (*Chr == '"')
					{
						OnAttribute(StoredAttributeName, FString(Chr - ContextPos - 1, ContextPos + 1));
						SubState = ESubState::Space;
					}
				}
				else if (SubState == ESubState::TagExit)
				{
					if (*Chr == '>')
					{
						OnTagClose();
						State = EState::Space;
					}
					else
					{
						OutError = "'>' expected";
						OutErrorLine = LineNumber;
						return false;
					}
				}
			}
			else if (State == EState::TagClose)
			{
				if (*Chr == '>')
				{
					OnTagClose();
					State = EState::Space;
				}
			}
			else if (State == EState::Comment)
			{
				if (*Chr == '>' && *(Chr - 1) == '-' && *(Chr - 2) == '-')
				{
					OnComment(FString(Chr - ContextPos - 6, ContextPos + 4), StateStartLineNumber);
					State = EState::Space;
				}
			}
			else if (State == EState::Text)
			{
				if (*Chr == '<')
				{
					OnText(FString(Chr - ContextPos, ContextPos));

					State = EState::Tag;
					SubState = ESubState::TagName;
					ContextPos = Chr;
				}
			}

			Chr++;
		}

		return true;
	}

private:
	static bool IsSpace(TCHAR Chr) { return Chr == ' ' || Chr == '\t' || Chr == '\r' || IsLineBreak(Chr); }
	static bool IsLineBreak(TCHAR Chr) { return Chr == '\n'; }
};
