/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#pragma once
#include <imgui.h>
extern "C" {
#include <lua.h>
}
#include <memory>
#include <thread>
#include <boost/filesystem.hpp>
#include <boost/thread/mutex.hpp>
#include "websocket.h"
#include "Preference.h"
#include <functional>
#include "OverlayContextBase.h"
#include <set>

class OverlayContextLua : public OverlayContextBase
{
protected:
	lua_State *L_script;
	virtual void Process(const std::string& message_str);
	bool is_loaded = false;
	boost::filesystem::path render_filename;
	std::mutex errors_mutex;
	std::set<std::string> errors;
public:
	lua_State *L_render;

public:
	OverlayContextLua();
	virtual ~OverlayContextLua();
	virtual bool Init(boost::filesystem::path path);
	virtual void Render(bool use_input, struct OverlayOption* options);
	virtual bool IsLoaded();
	virtual void Preferences();
	virtual void SetTexture(ImTextureID texture, const std::unordered_map<std::string, Image>* images);
	virtual void ToJson(Json::Value& setting) const {
		preference_storage.ToJson(setting["overlays"][name]);
	}
	virtual void FromJson(Json::Value& setting) {
		preference_storage.FromJson(setting["overlays"][name]);
	}
};

