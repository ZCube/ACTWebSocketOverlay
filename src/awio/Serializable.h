/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#pragma once 
#include <json/json.h>
#include <string>
#include <ImGui.h>

const std::string ImVec4TohtmlCode(ImVec4 val);
ImVec4 htmlCodeToImVec4(const std::string hex);

class Serializable
{
public:
	virtual void ToJson(Json::Value& value) const {};
	virtual void FromJson(Json::Value& value) {};
};

class Font : public Serializable
{
public:
	Font() {}
	Font(std::string fontname, std::string glyph_range, float font_size)
	{
		this->fontname = fontname;
		this->glyph_range = glyph_range;
		this->font_size = font_size;
	}
	std::string fontname;
	std::string glyph_range;
	float font_size;
	void ToJson(Json::Value& value) const
	{
		value["fontname"] = fontname;
		value["glyph_range"] = glyph_range;
		value["font_size"] = font_size;
	}
	void FromJson(Json::Value& value)
	{
		fontname = value["fontname"].asString();
		glyph_range = value["glyph_range"].asString();
		font_size = value["font_size"].asFloat();
	}
};

struct OverlayOption
{
	bool movable;
	bool show_preferences;
	std::map<std::string, ImVec2> windows_default_sizes;
	ImVec2& GetDefaultSize(const std::string& name, ImVec2 default)
	{
		auto it = windows_default_sizes.find(name);
		if (it != windows_default_sizes.end())
		{
			return it->second;
		}
		it->second = default;
		return default;
	}
};

class Image : public Serializable
{
public:
	int x;
	int y;
	int width;
	int height;
	ImVec2 uv0;
	ImVec2 uv1;
	std::vector<int> widths;
	std::vector<int> heights;
	void ToJson(Json::Value& value) const
	{
	}
	void FromJson(Json::Value& value)
	{
	}
};
