/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/
#pragma once
#include "OverlayContext.h"
#include <json/json.h>
#include "Table.h"
#include <boost/algorithm/string.hpp>

class AWIOOverlay : public OverlayContextBase
{
protected:
	ImTextureID texture;
	Json::Value processed_json;

	std::string Title;
	std::string zone;
	std::string duration;
	std::string rdps;
	std::string rhps;

	static std::map<std::string, std::string> name_to_job_map;
	static std::vector<const char*> combatant_attribute_names;
	static std::map<std::string, std::vector<std::string>> color_category_map;
	static std::string default_pet_job;
	const std::unordered_map<std::string, Image>* images = nullptr;

	float& global_opacity = preference_storage.float_map["Global"] = 1.0f;
	float& text_opacity = preference_storage.float_map["Text"] = 1.0f;
	float& background_opacity = preference_storage.float_map["Background"] = 0.2f;
	float& graph_opacity = preference_storage.float_map["Graph"] = 0.4f;
	std::string graph = "center";

	Table dealerTable= Table(
		global_opacity,
		text_opacity,
		graph_opacity,
		preference_storage.color_map["GraphText"],
		preference_storage.color_map["etc"],
		graph
	);

	std::string your_name;

	virtual void Process(const std::string& message_str);
public:
	AWIOOverlay();
	virtual ~AWIOOverlay();
	virtual bool Init(const boost::filesystem::path& path_);;
	virtual void Render(bool use_input, class OverlayOption* options);;
	virtual bool IsLoaded();
	virtual void Save();
	virtual void Load();
	virtual void ToJson(Json::Value& setting) const;
	virtual void FromJson(Json::Value& setting);
	virtual void PreferencesBody();
	virtual void SetTexture(ImTextureID texture, const std::unordered_map<std::string, Image>* images);
};