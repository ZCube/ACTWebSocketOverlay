/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#pragma once
extern "C"
{
#include <lua.h>
}
#include <string>
#include <json/json.h>

extern lua_State* lState; // lua state for imgui lua binding
void LoadImguiBindings(); // load lua binding
void ImLuaDraw(lua_State* L, bool use_input, const std::string& dir, const Json::Value& data);

extern "C" int jsonEncode(lua_State* L);
extern "C" int jsonEncodePretty(lua_State* L);
extern "C" int jsonDecode(lua_State* L);

int PushJsonToLuaTable(lua_State* L, const Json::Value& value);
void PopJsonFromLuaTable(lua_State* L, Json::Value& value);
