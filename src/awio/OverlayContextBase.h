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
#include <unordered_map>

class OverlayContextBase : public PreferenceBase
{
protected:
	bool loop = false;
	std::shared_ptr<websocket_context_base> websocket;
	std::shared_ptr<std::thread> websocket_thread;

	char websocket_message[1024] = { 0, };
	bool websocket_reconnect = true;
	
	bool current_websocket_ssl = false;
	std::string current_websocket_host="localhost";
	int current_websocket_port=0;
	std::string current_websocket_path="/";

	bool& visible =
		preference_storage.boolean_map["Visible"] = true;
	bool& websocket_ssl =
		preference_storage.boolean_map["WebsocketSSL"];
	std::string& websocket_host =
		preference_storage.string_map["WebsocketHost"];
	int& websocket_port =
		preference_storage.int_map["WebsocketPort"];
	std::string& websocket_path =
		preference_storage.string_map["WebsocketPath"];
public:
	bool& IsVisible()
	{
		return visible;
	}
	Json::Value processed_data;
	Json::Value last_processed_data;
	boost::mutex processed_data_mutex;
	bool processed_updated;
	std::string name;
	std::string id;
	std::string root_path_string;
	boost::filesystem::path root_path;

protected:
	virtual void Process(const std::string& message_str);
public:
	OverlayContextBase();
	virtual boost::filesystem::path GetImagesPath();
	virtual bool Init(const boost::filesystem::path& path) = 0;
	virtual void Render(bool use_input, class OverlayOption* options) = 0;
	virtual void WebSocketCheck();
	virtual void WebSocketRun();
	virtual void WebSocketStop();
	virtual bool IsLoaded() = 0;
	virtual void Preferences();

	virtual void PreferencesBody();
	virtual void SetTexture(ImTextureID texture, const std::unordered_map<std::string, Image>* images) = 0;
};
