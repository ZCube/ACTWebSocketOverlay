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
	Json::Reader r;
	Json::Value v, v2;

	if (r.parse(message_str, v))
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
					processed_data = v;
				}
			}

			PushJsonToLuaTable(L_script, v);
			int result = lua_pcall(L_script, 1, 1, 0);
			if (result) {
				char buf[1024] = { 0, };
				sprintf_s(buf, 1024, "Failed to run script: %s\n", lua_tostring(L_script, -1));
				errors_mutex.lock();
				errors.insert(buf);
				errors_mutex.unlock();
			}
			//if (lua_isstring(L_script, -1))
			{
				boost::mutex::scoped_lock l(processed_data_mutex);
				processed_updated = true;
				PopJsonFromLuaTable(L_script, v2);
				processed_data = v2;
			}
		}
		else
		{
			boost::mutex::scoped_lock l(processed_data_mutex);
			processed_updated = true;
			processed_data = v;
		}
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
extern "C" int saveWindowPos(lua_State* L);
extern "C" int getWindowSize(lua_State* L);
extern "C" int setWindowSize(lua_State* L);
extern "C" int getColor4(lua_State* L);
extern "C" int setColor4(lua_State* L);
extern "C" int getColor3(lua_State* L);
extern "C" int setColor3(lua_State* L);
extern "C" int getString(lua_State* L);
extern "C" int setString(lua_State* L);
extern "C" int getInt(lua_State* L);
extern "C" int setInt(lua_State* L);
extern "C" int getFloat(lua_State* L);
extern "C" int setFloat(lua_State* L);
extern "C" int getBoolean(lua_State* L);
extern "C" int setBoolean(lua_State* L);

PreferenceNode OverlayContextLua::LoadPreferenceNode(const std::string& name, const Json::Value& value)
{
	PreferenceNode node;
	static std::unordered_map<std::string, PreferenceNode::Type> types =
	{
		{ "Color3",PreferenceNode::Color },
		{ "Color4",PreferenceNode::Color4 },
		{ "Boolean",PreferenceNode::Boolean },
		{ "Int",PreferenceNode::Integer },
		{ "String",PreferenceNode::String },
		{ "Float",PreferenceNode::Float },
	};

	auto i = types.find(value["NodeType"].asString());
	if (i != types.end())
	{
		switch (i->second)
		{
		case PreferenceNode::Color:
			node.type = i->second;
			preference_storage.color_map[name];
			break;
		case PreferenceNode::Color4:
			node.type = i->second;
			preference_storage.color_map[name];
			break;
		case PreferenceNode::Boolean:
			node.type = i->second;
			preference_storage.boolean_map[name];
			break;
		case PreferenceNode::Integer:
			node.type = i->second;
			node.step = value.get("Step", 0).asInt();
			preference_storage.int_map[name];
			break;
		case PreferenceNode::String:
			node.type = i->second;
			node.string_max_length = value.get("StringMaxLength", 64).asInt();
			preference_storage.string_map[name].reserve(node.string_max_length);
			break;
		case PreferenceNode::Float:
			node.type = i->second;
			node.step = value.get("Step", 0).asFloat();
			preference_storage.float_map[name];
			break;
		};
	}
	for (auto j = value.begin(); j != value.end(); ++j)
	{
		if (j->isObject())
		{
			std::string key = j.key().asString();
			node.map[key] = LoadPreferenceNode(key, *j);
			node.UpdateMap();
		}
	}
	return node;
}

bool OverlayContextLua::Init(const boost::filesystem::path& path_) {
	boost::filesystem::path path = boost::filesystem::absolute(path_);
	root_path = path;
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

			Json::Value& websocket_default = value["WebSocketDefault"];
			if (!websocket_default.isNull())
			{
				websocket_path = websocket_default.get("Path", "/").asString();
				websocket_host = websocket_default.get("Host", "localhost").asString();
				if (websocket_host == "127.0.0.1")
				{
					websocket_host = "localhost";
				}
				websocket_port = websocket_default.get("Port", 0).asInt();
#if defined(USE_SSL)
				websocket_ssl = websocket_default.get("SSL", false).asBool();
#endif
			}

			Json::Value& preferences = value["Preferences"];
			preference_nodes.clear();
			if (!preferences.isNull())
			{
				for (auto i = preferences.begin(); i != preferences.end(); ++i)
				{
					preference_nodes.push_back(LoadPreferenceNode(i.key().asString(), *i));
					preference_nodes.back().name = i.key().asString();
				}
			}

			render_filename = path.filename() / "render.lua";

			if (boost::filesystem::exists(render_path))
			{
				L_render = luaL_newstate();   /* opens Lua */
				lState = L_render;
				LoadImguiBindings();
				_luaL_openlibs(L_render);
				lua_register(L_render, "jsonEncode", jsonEncode);
				lua_register(L_render, "jsonEncodePretty", jsonEncodePretty);
				lua_register(L_render, "jsonDecode", jsonDecode);
				lua_register(L_render, "getImage", getImage);
				lua_register(L_render, "saveWindowPos", saveWindowPos);
				lua_register(L_render, "getWindowSize", getWindowSize);
				lua_register(L_render, "setWindowSize", setWindowSize);
				
				lua_register(L_render, "getColor4", getColor4);
				lua_register(L_render, "setColor4", setColor4);
				lua_register(L_render, "getColor3", getColor3);
				lua_register(L_render, "setColor3", setColor3);
				lua_register(L_render, "getString", getString);
				lua_register(L_render, "setString", setString);
				lua_register(L_render, "getInt", getInt);
				lua_register(L_render, "setInt", setInt);
				lua_register(L_render, "getFloat", getFloat);
				lua_register(L_render, "setFloat", setFloat);
				lua_register(L_render, "getBoolean", getBoolean);
				lua_register(L_render, "setBoolean", setBoolean);

				lua_pushlightuserdata(L_render, &preference_storage);
				lua_setglobal(L_render, "storage");

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
				lua_register(L_script, "jsonEncode", jsonEncode);
				lua_register(L_script, "jsonEncodePretty", jsonEncodePretty);
				lua_register(L_script, "jsonDecode", jsonDecode);

				lua_register(L_script, "getColor4", getColor4);
				lua_register(L_script, "setColor4", setColor4);
				lua_register(L_script, "getColor3", getColor3);
				lua_register(L_script, "setColor3", setColor3);
				lua_register(L_script, "getString", getString);
				lua_register(L_script, "setString", setString);
				lua_register(L_script, "getInt", getInt);
				lua_register(L_script, "setInt", setInt);
				lua_register(L_script, "getFloat", getFloat);
				lua_register(L_script, "setFloat", setFloat);
				lua_register(L_script, "getBoolean", getBoolean);
				lua_register(L_script, "setBoolean", setBoolean);

				lua_pushlightuserdata(L_script, &preference_storage);
				lua_setglobal(L_script, "storage");

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

void OverlayContextLua::Render(bool use_input, class OverlayOption* options)
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
