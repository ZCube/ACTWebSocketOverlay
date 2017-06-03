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
extern lua_State* lState; // lua state for imgui lua binding
void LoadImguiBindings(); // load lua binding
void ImLuaDraw(lua_State* L, bool use_input, const std::string& dir, const std::string& data);