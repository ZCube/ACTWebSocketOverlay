/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include "version.h"
#include <imgui.h>
#include "imgui_internal.h"
#include "imgui_lua.h"
#include <stb_image.h>
#include <json/json.h>
#include <functional>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <stdio.h>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <thread>
#include <unordered_map>

#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <beast/core.hpp>
#include <beast/websocket.hpp>
#if defined(USE_SSL)
#include <beast/websocket/ssl.hpp>
#include <boost/asio/ssl.hpp>
#endif
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <chrono>

#include <atlstr.h>
#define STR2UTF8(s) (CW2A(CA2W(s), CP_UTF8))
#define UTF82WSTR(s) (CA2W(s), CP_UTF8)

#include "OverlayContext.h"
#include "OverlayContextLua.h"

const luaL_Reg loadedlibs[12] = {
	{ "_G", luaopen_base },
	{ LUA_LOADLIBNAME, luaopen_package },
	{ LUA_COLIBNAME, luaopen_coroutine },
	{ LUA_TABLIBNAME, luaopen_table },
	{ LUA_OSLIBNAME, luaopen_os },
	// { LUA_IOLIBNAME, luaopen_io },  // disable file io <- may be danger.
	{ LUA_STRLIBNAME, luaopen_string },
	{ LUA_MATHLIBNAME, luaopen_math },
	{ LUA_UTF8LIBNAME, luaopen_utf8 },
	{ LUA_DBLIBNAME, luaopen_debug },
#if defined(LUA_COMPAT_BITLIB)
	{ LUA_BITLIBNAME, luaopen_bit32 },
#endif
	{ NULL, NULL }
};


void _luaL_openlibs(lua_State * L) {
	const luaL_Reg *lib;
	/* "require" functions from 'loadedlibs' and set results to global table */
	for (lib = loadedlibs; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}
}


std::mutex unique_count_mutex;
int unique_id = 0;
std::unordered_map<std::string, int> unique_count;

void ClearUniqueValues()
{
	unique_count_mutex.lock();
	unique_count.clear();
	unique_id = 0;
	unique_count_mutex.unlock();
}

std::string UniqueName(std::string name)
{
	std::string ret;
	unique_count_mutex.lock();
	++unique_count[name];
	if (unique_count[name] > 1)
	{
		char buf[10] = { 0, };
		_itoa_s(unique_count[name], buf, 10);
		ret = name + "_" + buf;
	}
	else
	{
		ret = name;
	}
	unique_count_mutex.unlock();
	return ret;
}

std::string UniqueID()
{
	std::string ret;
	unique_count_mutex.lock();
	char buf[10] = { 0, };
	_itoa_s(unique_id++, buf, 10);
	ret = std::string(buf) + ".";
	unique_count_mutex.unlock();
	return ret;
}

OverlayContextLua::OverlayContextLua() {
	loop = false;
	processed_updated = true;
	websocket_ssl = false;
	websocket_host = "localhost";
	websocket_port = 10501;
	websocket_path = "/";

	websocket_ssl = true;
	websocket_host = "localhost";
	websocket_port = 10501;
	websocket_path = "/MiniParse";

	if (websocket_thread)
	{
		websocket_thread->join();
		websocket_thread.reset();
	}
}

OverlayContextLua::~OverlayContextLua() {
	//Save();
	WebSocketStop();
	if (L_render)
		lua_close(L_render);
	if (L_script)
		lua_close(L_script);
}

void OverlayContextLua::Process(const std::string & message_str)
{
	if (L_script)
	{
		lua_getglobal(L_script, "script");
		if (!lua_isfunction(L_script, -1))
		{
			lua_pop(L_script, 1);
			{
				boost::mutex::scoped_lock l(processed_data_mutex);
				processed_updated = true;
				processed_data = message_str;
			}
		}

		lua_pushlstring(L_script, message_str.c_str(), message_str.size());
		int result = lua_pcall(L_script, 1, 1, 0);
		if (result) {
			char buf[1024] = { 0, };
			sprintf_s(buf, 1024, "Failed to run script: %s\n", lua_tostring(L_script, -1));
			errors_mutex.lock();
			errors.insert(buf);
			errors_mutex.unlock();
		}
		if (lua_isstring(L_script, -1))
		{
			boost::mutex::scoped_lock l(processed_data_mutex);
			processed_updated = true;
			processed_data = lua_tostring(L_script, -1);
		}
	}
	else
	{
		boost::mutex::scoped_lock l(processed_data_mutex);
		processed_updated = true;
		processed_data = message_str;
	}
}

bool OverlayContextLua::IsLoaded() { return is_loaded; }

void OverlayContextLua::SetTexture(ImTextureID texture, const std::unordered_map<std::string, Image>* images)
{
	if (L_render)
	{
		lua_pushinteger(L_render, (lua_Integer)texture);
		lua_setglobal(L_render, "texture_id");
	}
}

extern "C" int getImage(lua_State* L);
extern "C" int getWindowSize(lua_State* L);
extern "C" int setWindowSize(lua_State* L);

bool OverlayContextLua::Init(boost::filesystem::path path) {
	path = boost::filesystem::absolute(path);
	auto script_path = path / "script.json";

	if (boost::filesystem::exists(script_path))
	{
		Json::Value value;
		Json::Reader reader;
		std::ifstream fin(script_path.wstring().c_str());
		if (reader.parse(fin, value))
		{
			name = UniqueName(value["Name"].asString());
			id = UniqueID();
			auto render_path = (path / "render.lua");
			auto script_path = (path / "script.lua");
			auto preference_path = (path / "preference.lua");

			render_filename = path.filename() / "render.lua";

			if (boost::filesystem::exists(render_path))
			{
				L_render = luaL_newstate();   /* opens Lua */
				lState = L_render;
				LoadImguiBindings();
				_luaL_openlibs(L_render);
				lua_register(L_render, "getImage", getImage);
				lua_register(L_render, "getWindowSize", getImage);
				lua_register(L_render, "setWindowSize", getImage);

				lua_pushlstring(L_render, path.string().c_str(), path.string().size());
				lua_setglobal(L_render, "global_path");
				luaL_dostring(L_render, "package.path = package.path .. ';' .. global_path .. '/?.lua'");


				lua_pushlstring(L_render, name.c_str(), name.size());
				lua_setglobal(L_render, "window_id");

				std::ifstream t(render_path.wstring().c_str());
				std::string render((std::istreambuf_iterator<char>(t)),
					std::istreambuf_iterator<char>());
				is_loaded = true;

				std::string u8name = STR2UTF8((path.filename() / render_path.filename()).string().c_str());
				int status = luaL_loadbufferx(L_render, render.c_str(), render.size(), u8name.c_str(), NULL);
				if (status) {
					char buf[1024] = { 0, };
					sprintf_s(buf, 1024, "Couldn't load string: %s\n", lua_tostring(L_render, -1));
					errors_mutex.lock();
					errors.insert(buf);
					errors_mutex.unlock();
				}
				int result = lua_pcall(L_render, 0, 0, 0);
				if (result) {
					char buf[1024] = { 0, };
					sprintf_s(buf, 1024, "Failed to run script: %s\n", lua_tostring(L_render, -1));
					errors_mutex.lock();
					errors.insert(buf);
					errors_mutex.unlock();
				}
			}
			else
			{
				L_render = nullptr;
			}

			if (boost::filesystem::exists(script_path))
			{
				L_script = luaL_newstate();   /* opens Lua */
				_luaL_openlibs(L_script);
				lua_pushlstring(L_script, path.string().c_str(), path.string().size());
				lua_setglobal(L_script, "global_path");
				luaL_dostring(L_script, "package.path = package.path .. ';' .. global_path .. '/?.lua'");

				std::ifstream t(script_path.wstring().c_str());
				std::string script((std::istreambuf_iterator<char>(t)),
					std::istreambuf_iterator<char>());

				if (!script.empty())
				{
					std::string u8name = STR2UTF8((path.filename() / script_path.filename()).string().c_str());
					int status = luaL_loadbufferx(L_script, script.c_str(), script.size(), u8name.c_str(), NULL);
					if (status) {
						char buf[1024] = { 0, };
						sprintf_s(buf, 1024, "Couldn't load string: %s\n", lua_tostring(L_script, -1));
						errors_mutex.lock();
						errors.insert(buf);
						errors_mutex.unlock();
					}
					int result = lua_pcall(L_script, 0, 0, 0);
					if (result) {
						char buf[1024] = { 0, };
						sprintf_s(buf, 1024, "Failed to run script: %s\n", lua_tostring(L_script, -1));
						errors_mutex.lock();
						errors.insert(buf);
						errors_mutex.unlock();
					}
				}
				else {
					L_script = nullptr;
				}
			}
		}
		else
		{
			// TODO : 
			// parsing error
		}
		//scripts[boost::filesystem::exists]
	}
	return true;
}

void OverlayContextLua::Render(bool use_input, struct OverlayOption* options)
{
	if (IsLoaded() && L_render)
	{
		if (processed_updated && processed_data_mutex.try_lock())
		{
			last_processed_data = processed_data;
			processed_updated = false;
			processed_data_mutex.unlock();
		}
		ImGuiContext& g = *ImGui::GetCurrentContext();
		bool restore_accessed = false;
		restore_accessed = (g.CurrentWindow && !g.CurrentWindow->Accessed);
		ImGui::PushID(name.c_str());
		if (restore_accessed)
			g.CurrentWindow->Accessed = false;

		ImLuaDraw(L_render, use_input, render_filename.string().c_str(), last_processed_data);

		restore_accessed = (g.CurrentWindow && !g.CurrentWindow->Accessed);
		ImGui::PopID();
		if (restore_accessed)
			g.CurrentWindow->Accessed = false;
	}

	errors_mutex.lock();
	if (errors.size() > 0)
	{
		for (auto i = errors.begin(); i != errors.end(); ++i)
		{
			ImGui::Text(i->c_str());
		}
	}
	errors_mutex.unlock();
}

void OverlayContextLua::Preferences()
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
		ImGui::Separator();
		for (auto i = preference_nodes.begin(); i != preference_nodes.end(); ++i)
		{
			PreferenceBase::Preferences(*i, preference_storage);
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}
