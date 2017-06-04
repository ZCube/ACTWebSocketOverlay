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

protected:
	virtual void Process(const std::string& message_str);
public:
	OverlayContextBase() {
		websocket_ssl = false;
		websocket_host = "localhost";
		websocket_port = 0;
		websocket_path = "/";
	}
	virtual bool Init(boost::filesystem::path path) = 0;
	virtual void Render(bool use_input, struct OverlayOption* options) = 0;
	virtual void WebSocketRun();
	virtual void WebSocketStop()
	{
		loop = false;
		if (websocket_thread)
		{
			websocket_thread->join();
			websocket_thread.reset();
		}
	}
	virtual bool IsLoaded() = 0;
	virtual void Preferences()
	{
		ImGui::PushID(name.c_str());
		if (ImGui::TreeNode(name.c_str(), "%s : %s", name.c_str(), websocket_message))
		{
			ImGui::Separator();
			if (ImGui::TreeNode("WebSocket", "WebSocket : %s://%s:%d%s",
#if defined(USE_SSL)
				websocket_ssl ? "wss" :
#endif
				"ws", websocket_host.c_str(), websocket_port, websocket_path.c_str()))
			{
#if defined(USE_SSL)
				if (ImGui::Checkbox("Use SSL", &websocket_ssl))
				{
					websocket_reconnect = true;
					if (websocket)
					{
						websocket->close(beast::websocket::close_reason(0));
					}
					strcpy_s(websocket_message, 1023, "Connecting...");
					Save();
				}
#endif
				{
					websocket_host.reserve(65);
					if (ImGui::InputText("Host", (char*)websocket_host.data(), 64))
					{
						int len = strnlen_s(websocket_host.data(), 64);
						websocket_host.resize(len);

						websocket_reconnect = true;
						if (websocket)
						{
							websocket->close(beast::websocket::close_reason(0));
						}
						strcpy_s(websocket_message, 1023, "Connecting...");
						Save();
					}
				}
				{
					websocket_path.reserve(1024);
					websocket_path[0] = '/';
					if (ImGui::InputText("Path", (char*)websocket_path.data() + 1, 1022))
					{
						int len = strnlen_s(websocket_path.data() + 1, 1022);
						websocket_path.resize(len + 1);

						websocket_reconnect = true;
						if (websocket)
						{
							websocket->close(beast::websocket::close_reason(0));
						}
						strcpy_s(websocket_message, 1023, "Connecting...");
						Save();
					}
				}
				if (ImGui::InputInt("Port", &websocket_port))
				{
					websocket_reconnect = true;
					if (websocket)
					{
						websocket->close(beast::websocket::close_reason(0));
					}
					strcpy_s(websocket_message, 1023, "Connecting...");
					Save();
				}
				ImGui::TreePop();
			}
			PreferencesBody();
			ImGui::Separator();
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	virtual void PreferencesBody()
	{
		for (auto i = preference_nodes.begin(); i != preference_nodes.end(); ++i)
		{
			PreferenceBase::Preferences(*i, preference_storage);
		}
	}
	virtual void SetTexture(ImTextureID texture, const std::unordered_map<std::string, Image>* images) = 0;
};
