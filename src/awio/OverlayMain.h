/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#pragma once

#include "version.h"
#include <imgui.h>
#include "imgui_lua.h"
#include <Windows.h>
#include <unordered_map>
#include <atlstr.h>
#include <boost/algorithm/string.hpp>
#define STR2UTF8(s) (CW2A(CA2W(s), CP_UTF8))
#define UTF82WSTR(s) (CA2W(s), CP_UTF8)

#include "Serializable.h"
#include "Preference.h"
#include "OverlayContext.h"
#include "AWIOOverlay.h"

class OverlayInstance : public PreferenceBase
{
public:
	OverlayOption options = {
		preference_storage.boolean_map["Movable"],
		preference_storage.boolean_map["ShowPreferences"],
	};
protected:
	boost::mutex m;
	std::unordered_map<std::string, std::shared_ptr<OverlayContextBase> > overlays;
	std::vector<std::pair<std::string, ImVec4*>> style_colors;
	boost::filesystem::path root_path;
	boost::filesystem::path setting_path;

	bool initialized = false;
	
	float font_sizes = 13.0f;
	// font setting
	std::vector<Font> fonts = {
		Font("consolab.ttf", "Default", font_sizes),
		Font("Default", "Default", font_sizes), // Default will ignore font size. 13
		Font("ArialUni.ttf", "Japanese", font_sizes),
		Font("NanumBarunGothic.ttf", "Korean", font_sizes),
		Font("gulim.ttc", "Korean", font_sizes),
	};

	std::vector<const char*> glyph_range_key = {
		"Default",
		"Chinese",
		"Cyrillic",
		"Japanese",
		"Korean",
		"Thai",
	};

public:
	std::unordered_map<std::string, Image> overlay_images;
	ImTextureID overlay_texture = nullptr;
	unsigned char *overlay_texture_filedata = nullptr;
	int overlay_texture_width = 0, overlay_texture_height = 0, overlay_texture_channels = 0;

public:

	void SetTexture(ImTextureID texture);
	OverlayInstance();
	~OverlayInstance();

	void FontsPreferences();
	virtual void Preferences();

	std::vector<boost::filesystem::path> font_paths;
	std::vector<std::string> font_filenames;
	std::vector<const char *> font_cstr_filenames;
	PreferenceStorage* current_storage = nullptr;
	ImGuiContext* context;

	ImGuiStyle imgui_style;
	bool font_setting_dirty = false;

	void LoadFonts();
	void LoadOverlays();
	void LoadTexture();
	void ReloadScripts();
	void UnloadScripts();
	void Init(ImGuiContext* context, const boost::filesystem::path& path);
	void Render(ImGuiContext* context);
	void Load();
	void Save();
};
