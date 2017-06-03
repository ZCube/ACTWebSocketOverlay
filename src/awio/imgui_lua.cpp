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

void ImLuaDraw(lua_State* L,bool use_input, const std::string& dir,const std::string& data)
{
	lState = L;
	lua_getglobal(L, "render");
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return;
	}
	lua_pushboolean(L, use_input);
	lua_pushlstring(L, data.c_str(), data.size());

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
