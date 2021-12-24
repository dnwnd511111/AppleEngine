#include "apSpriteFont.h"
#include "apHelper.h"

using namespace ap::graphics;

namespace ap
{

	void SpriteFont::FixedUpdate()
	{
		if (IsDisableUpdate())
			return;
	}
	void SpriteFont::Update(float dt)
	{
		if (IsDisableUpdate())
			return;
	}

	void SpriteFont::Draw(CommandList cmd) const
	{
		if (IsHidden())
			return;
		ap::font::Draw(text, params, cmd);
	}

	float SpriteFont::TextWidth() const
	{
		return ap::font::TextWidth(text, params);
	}
	float SpriteFont::TextHeight() const
	{
		return ap::font::TextHeight(text, params);
	}

	void SpriteFont::SetText(const std::string& value)
	{
		ap::helper::StringConvert(value, text);
	}
	void SpriteFont::SetText(std::string&& value)
	{
		ap::helper::StringConvert(value, text);
	}
	void SpriteFont::SetText(const std::wstring& value)
	{
		text = value;
	}
	void SpriteFont::SetText(std::wstring&& value)
	{
		text = std::move(value);
	}

	std::string SpriteFont::GetTextA() const
	{
		std::string retVal;
		ap::helper::StringConvert(text, retVal);
		return retVal;
	}
	const std::wstring& SpriteFont::GetText() const
	{
		return text;
	}

}
