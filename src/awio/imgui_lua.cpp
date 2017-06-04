/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_lua_bindings.cpp>
#include <json/json.h>

int PushJsonToLuaTable(lua_State* L, const Json::Value& value)
{
	switch (value.type())
	{
	case Json::ValueType::nullValue: ///< 'null' value
		lua_pushnil(L);
		break;
	case Json::ValueType::intValue:      ///< signed integer value
		lua_pushnumber(L, value.asInt64());
		break;
	case Json::ValueType::uintValue:     ///< unsigned integer value
		lua_pushnumber(L, value.asUInt64());
		break;
	case Json::ValueType::realValue:     ///< double value
		lua_pushnumber(L, value.asDouble());
		break;
	case Json::ValueType::stringValue:   ///< UTF-8 string value
		lua_pushstring(L, value.asCString());
		break;
	case Json::ValueType::booleanValue:  ///< bool value
		lua_pushboolean(L, value.asBool());
		break;
	case Json::ValueType::arrayValue:    ///< array value (ordered list)
	{
		lua_createtable(L, value.size(), 0);
		int idx = 0;
		for (auto i = value.begin(); i != value.end(); ++i, ++idx)
		{
			PushJsonToLuaTable(L, *i);
			lua_rawseti(L, -2, idx);
		}
	}
	break;
	case Json::ValueType::objectValue:    ///< object value (collection of name/value pairs).
	{
		lua_createtable(L, value.size(), 0);
		int idx = 1;
		for (auto i = value.begin(); i != value.end(); ++i, ++idx)
		{
			const std::string& key = i.key().asString();
			lua_pushlstring(L, key.data(), key.size());
			PushJsonToLuaTable(L, *i);
			lua_settable(L, -3);
		}
	}
	break;
	}
	/* Returning one table which is already on top of Lua stack. */
	return 1;
}

void PopJsonFromLuaTable(lua_State* L, Json::Value& value)
{
	int type = lua_type(L, -1);
	switch (type)
	{
	case LUA_TBOOLEAN:
		value = (bool)lua_toboolean(L, -1);
		return;
	case LUA_TNUMBER:
		if (lua_isinteger(L, -1))
		{
			value = lua_tointeger(L, -1);
		}
		if (lua_isnumber(L, -1))
		{
			value = lua_tonumber(L, -1);
		}
		return;
	case LUA_TNIL:
		return;
	case LUA_TSTRING:
		value = lua_tostring(L, -1);
		return;
	case LUA_TTABLE:
		break;
	}

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		int keytype = lua_type(L, -2);
		int valuetype = lua_type(L, -1);
		switch (keytype)
		{
		case LUA_TNUMBER:
		{
			const int key = luaL_checknumber(L, -2);
			PopJsonFromLuaTable(L, value[key]);
		}
		break;
		case LUA_TNIL:
			// TODO how?
			break;
		case LUA_TSTRING:
		{
			const std::string key = luaL_checkstring(L, -2);
			PopJsonFromLuaTable(L, value[key]);
		}
		break;

		}
		lua_pop(L, 1);
	}
	return;
}


extern "C" int jsonDecode(lua_State* L)
{
	const char* str = luaL_checkstring(L, 1);
	if (str)
	{
		Json::Reader r;
		Json::Value v;
		if (r.parse(str, v))
		{
		}
		PushJsonToLuaTable(L, v);
	}
	return 6;
}

extern "C" int jsonEncode(lua_State* L)
{
	Json::FastWriter w;
	Json::Value v;
	PopJsonFromLuaTable(L, v);
	std::string str = w.write(v);
	lua_pushlstring(L, str.c_str(), str.size());
	return 1;
}

extern "C" int jsonEncodePretty(lua_State* L)
{
	Json::StyledWriter w;
	Json::Value v;
	PopJsonFromLuaTable(L, v);
	std::string str = w.write(v);
	lua_pushlstring(L, str.c_str(), str.size());
	return 1;
}

void ImLuaDraw(lua_State* L,bool use_input, const std::string& dir,const Json::Value& data)
{
	lState = L;
	lua_getglobal(L, "render");
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return;
	}
	lua_pushboolean(L, use_input);
	PushJsonToLuaTable(L, data);

#ifdef ENABLE_IM_LUA_END_STACK
	endStack.clear();
#endif
	int result = lua_pcall(L, 2, 0, 0);
#ifdef ENABLE_IM_LUA_END_STACK
	bool wasEmpty = endStack.empty();
	while (!endStack.empty()) {
		ImEndStack(endStack.back());
		endStack.pop_back();
	}

#endif
#ifdef ENABLE_IM_LUA_END_STACK
	if (!wasEmpty) {
		ImGui::Text("Script %s didn't clean up imgui stack properly", dir.c_str());
	}
#endif
	if (result) {
		ImGui::Text("Failed to run script: %s\n", lua_tostring(L, -1));
	}
}
