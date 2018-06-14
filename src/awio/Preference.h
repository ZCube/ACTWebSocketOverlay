/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#pragma once
#include "Serializable.h"
#include <imgui.h>
#include <map>
#include <string>

class PreferenceStorage
{
public:
	std::map<std::string, std::string> string_map;
	std::map<std::string, ImVec4> color_map;
	std::map<std::string, bool> boolean_map;
	std::map<std::string, float> float_map;
	std::map<std::string, int> int_map;
	void ToJson(Json::Value& value) const
	{
		Json::Value color;
		for (auto i = color_map.begin(); i != color_map.end(); ++i)
		{
			color[i->first] = ImVec4TohtmlCode(i->second);
		}
		value["color_map"] = color;

		Json::Value floatval;
		for (auto i = float_map.begin(); i != float_map.end(); ++i)
		{
			floatval[i->first] = i->second;
		}
		value["float_map"] = floatval;

		Json::Value boolean;
		for (auto i = boolean_map.begin(); i != boolean_map.end(); ++i)
		{
			boolean[i->first] = i->second;
		}
		value["boolean_map"] = boolean;

		Json::Value intval;
		for (auto i = int_map.begin(); i != int_map.end(); ++i)
		{
			intval[i->first] = i->second;
		}
		value["int_map"] = intval;

		Json::Value string_map;
		for (auto i = this->string_map.begin(); i != this->string_map.end(); ++i)
		{
			string_map[i->first] = i->second;
		}
		value["string_map"] = string_map;

	}
	void FromJson(Json::Value& value)
	{
		// global
		Json::Value color = value.get("color_map", Json::Value());
		for (auto i = color.begin(); i != color.end(); ++i)
		{
			color_map[i.key().asString()] = htmlCodeToImVec4(i->asString());
		}
		// global
		Json::Value floatval = value.get("float_map", Json::Value());
		for (auto i = floatval.begin(); i != floatval.end(); ++i)
		{
			float_map[i.key().asString()] = i->asFloat();
		}
		// global
		Json::Value boolean = value.get("boolean_map", Json::Value());
		for (auto i = boolean.begin(); i != boolean.end(); ++i)
		{
			boolean_map[i.key().asString()] = i->asFloat();
		}
		// global
		Json::Value intval = value.get("int_map", Json::Value());
		for (auto i = intval.begin(); i != intval.end(); ++i)
		{
			int_map[i.key().asString()] = i->asInt();
		}
		// global
		Json::Value string = value.get("string_map", Json::Value());
		for (auto i = string.begin(); i != string.end(); ++i)
		{
			string_map[i.key().asString()] = i->asString();
		}
	}
};

class PreferenceNode
{
public:
	enum Type
	{
		Integer, // int
		Float, // float 
		Boolean, // boolean
		Color, // ImVec4
		Color4, // ImVec4
		String, // std::string
		Map,
	};
	PreferenceNode()
	{

	}
	PreferenceNode(const Image& image_, const std::string& name_, const std::vector<PreferenceNode>& vec)
		: image(image_)
		, name(name_)
	{
		for (auto i = vec.begin(); i != vec.end(); ++i)
		{
			map[i->name] = *i;
		}
	}
	PreferenceNode(const std::string& name_, const std::vector<PreferenceNode>& vec)
		: name(name_)
	{
		for (auto i = vec.begin(); i != vec.end(); ++i)
		{
			map[i->name] = *i;
		}
	}
	PreferenceNode(const Image& image_, const std::string& name_, Type type_, void* val_ = nullptr, int string_max_length_ = 0)
		: image(image_)
		, name(name_)
		, val(val_)
		, type(type_)
		, string_max_length(string_max_length_)
	{

	}
	PreferenceNode(const std::string& name_, Type type_, void* val_ = nullptr, int string_max_length_ = 0)
		: name(name_)
		, type(type_)
		, val(val_)
		, string_max_length(string_max_length_)
	{

	}
	void UpdateMap()
	{
		for (auto i = map.begin(); i != map.end(); ++i)
		{
			i->second.name = i->first;
		}
	}
	Image image;
	Type type;
	void* val = nullptr;
	int string_max_length = 64;
	std::string name;
	std::map<std::string, PreferenceNode> map;
	float step = 0;
};

class PreferenceBase : public Serializable
{
protected:
	PreferenceBase* root = nullptr;
public:
	void* instance = nullptr;
	std::vector<PreferenceNode> preference_nodes;
	PreferenceStorage preference_storage;
	virtual void SetRoot(PreferenceBase* root);
	virtual void Save();
	virtual void Load();
	virtual void Preferences(PreferenceNode& preference_node, PreferenceStorage& preference_storage);
	virtual void Preferences();
};
