/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include "version.h"
#include <imgui.h>
#include "imgui_lua.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
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

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

void ClearUniqueValues();

#include <imgui_internal.h>
#include "OverlayMain.h"

#include <boost/thread/recursive_mutex.hpp>
boost::recursive_mutex instanceLock;
static OverlayInstance instance;

extern "C" int getWindowSize(lua_State* L)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	const char* str = luaL_checkstring(L, 1);
	if (str)
	{
		auto i = instance.options.windows_default_sizes.find(str);
		if (i != instance.options.windows_default_sizes.end())
		{
			lua_pushnumber(L, i->second.x);
			lua_pushnumber(L, i->second.y);
		}
		else
		{
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
		}
	}
	else
	{
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
	}
	return 2;
}
extern "C" int setWindowSize(lua_State* L)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	const char* str = luaL_checkstring(L, 1);
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	if (str)
	{
		auto i = instance.options.windows_default_sizes.find(str);
		if (i != instance.options.windows_default_sizes.end())
		{
			i->second.x = x;
			i->second.y = y;
		}
	}
	return 0;
}

extern "C" int getImage(lua_State* L)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	const char* str = luaL_checkstring(L, 1);
	if (str)
	{
		auto i = instance.overlay_images.find(str);
		if (i != instance.overlay_images.end())
		{
			lua_pushnumber(L, i->second.width);
			lua_pushnumber(L, i->second.height);
			lua_pushnumber(L, i->second.uv0.x);
			lua_pushnumber(L, i->second.uv0.y);
			lua_pushnumber(L, i->second.uv1.x);
			lua_pushnumber(L, i->second.uv1.y);
		}
		else
		{
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);
		}
	}
	return 6;
}

extern "C" int getColor4(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->color_map.find(str);
		if (i != storage->color_map.end())
		{
			ImVec4& v = i->second;
			lua_pushnumber(L, v.x);
			lua_pushnumber(L, v.y);
			lua_pushnumber(L, v.z);
			lua_pushnumber(L, v.w);
			return 4;
		}
	}
	ImVec4 v(0,0,0,1);
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	lua_pushnumber(L, v.z);
	lua_pushnumber(L, v.w);
	return 4;
}
extern "C" int setColor4(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str &&
		lua_isnumber(L, 3) &&
		lua_isnumber(L, 4) &&
		lua_isnumber(L, 5) &&
		lua_isnumber(L, 6) &&
		storage)
	{
		auto i = storage->color_map.find(str);
		if (i != storage->color_map.end())
		{
			const float v0 = luaL_checknumber(L, 3);
			const float v1 = luaL_checknumber(L, 4);
			const float v2 = luaL_checknumber(L, 5);
			const float v3 = luaL_checknumber(L, 6);
			i->second.x = v0;
			i->second.y = v1;
			i->second.z = v2;
			i->second.w = v3;
			return 0;
		}
	}
	return 0;
}
extern "C" int getColor3(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->color_map.find(str);
		if (i != storage->color_map.end())
		{
			ImVec4& v = i->second;
			lua_pushnumber(L, v.x);
			lua_pushnumber(L, v.y);
			lua_pushnumber(L, v.z);
			return 3;
		}
	}
	ImVec4 v(0, 0, 0, 1);
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	lua_pushnumber(L, v.z);
	return 3;
}
extern "C" int setColor3(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str &&
		lua_isnumber(L, 3) &&
		lua_isnumber(L, 4) &&
		lua_isnumber(L, 5) &&
		storage)
	{
		auto i = storage->color_map.find(str);
		if (i != storage->color_map.end())
		{
			const float v0 = luaL_checknumber(L, 3);
			const float v1 = luaL_checknumber(L, 4);
			const float v2 = luaL_checknumber(L, 5);
			i->second.x = v0;
			i->second.y = v1;
			i->second.z = v2;
			return 0;
		}
	}
	return 0;
}
extern "C" int getString(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->string_map.find(str);
		if (i != storage->string_map.end())
		{
			std::string& v = i->second;
			lua_pushlstring(L, v.c_str(), v.size());
			return 1;
		}
	}
	std::string v;
	lua_pushlstring(L, v.c_str(), v.size());
	return 1;
}
extern "C" int setString(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	const char* v = luaL_checkstring(L, 3);
	if (v && str && storage)
	{
		auto i = storage->string_map.find(str);
		if (i != storage->string_map.end())
		{
			i->second = v;
			return 0;
		}
	}
	return 0;
}
extern "C" int getInt(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->int_map.find(str);
		if (i != storage->int_map.end())
		{
			int& v = i->second;
			lua_pushnumber(L, v);
			return 1;
		}
	}
	int v = 0;
	lua_pushnumber(L, v);
	return 1;
}
extern "C" int setInt(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (lua_isnumber(L, 3) && str && storage)
	{
		const int v = lua_tonumber(L, 3);
		auto i = storage->int_map.find(str);
		if (i != storage->int_map.end())
		{
			i->second = v;
			return 0;
		}
	}
	return 0;
}
extern "C" int getFloat(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->float_map.find(str);
		if (i != storage->float_map.end())
		{
			float& v = i->second;
			lua_pushnumber(L, v);
			return 1;
		}
	}
	float v = 0.0f;
	lua_pushnumber(L, v);
	return 1;
}
extern "C" int setFloat(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (lua_isnumber(L, 3) && str && storage)
	{
		const float v = lua_tonumber(L, 3);
		auto i = storage->float_map.find(str);
		if (i != storage->float_map.end())
		{
			i->second = v;
			return 0;
		}
	}
	return 0;
}
extern "C" int getBoolean(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (str && storage)
	{
		auto i = storage->boolean_map.find(str);
		if (i != storage->boolean_map.end())
		{
			bool& v = i->second;
			lua_pushboolean(L, v);
			return 1;
		}
	}
	bool v = false;
	lua_pushboolean(L, v);
	return 1;
}
extern "C" int setBoolean(lua_State* L)
{
	PreferenceStorage* storage = lua_islightuserdata(L, 1) ? reinterpret_cast<PreferenceStorage*>(lua_touserdata(L, 1)) : nullptr;
	const char* str = luaL_checkstring(L, 2);
	if (lua_isboolean(L, 3) && str && storage)
	{
		const bool v = lua_toboolean(L, 3);
		auto i = storage->boolean_map.find(str);
		if (i != storage->boolean_map.end())
		{
			i->second = v;
			return 0;
		}
	}
	return 0;
}

extern "C" int ModInit(ImGuiContext* context)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	ImGui::SetCurrentContext(context);
	ImGui::GetIO().Fonts->AddFontDefault();

	WCHAR result[MAX_PATH] = {};
	GetModuleFileNameW(NULL, result, MAX_PATH);
	boost::filesystem::path module_path = result;
	boost::filesystem::path module_dir = module_path.parent_path();

	instance.Init(context, module_dir);

	return 0;
}

extern "C" int ModUnInit(ImGuiContext* context)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	try {
		instance.UnloadScripts();
	}
	catch (...)
	{

	}
	return 0;
}

extern "C" void ModTextureData(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	assert(out_pixels != nullptr && out_width != nullptr && out_height != nullptr);
	//*out_pixels = nullptr;
	//*out_width = 0;
	//*out_height = 0;
	//return;
	*out_pixels = instance.overlay_texture_filedata;
	*out_width = instance.overlay_texture_width;
	*out_height = instance.overlay_texture_height;
	if (out_bytes_per_pixel)
		*out_bytes_per_pixel = instance.overlay_texture_channels;
}

extern "C" void ModSetTexture(void* texture)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	instance.SetTexture(texture);
	if (texture == nullptr)
	{
	}
}

extern "C" int ModRender(ImGuiContext* context)
{
	boost::recursive_mutex::scoped_lock l(instanceLock);
	instance.Render(context);
	ImGuiContext& g = *context;
	if (g.CurrentWindow && g.CurrentWindow->Accessed)
	{
		// debug
		if (ImGui::Button("Reload Overlays"))
		{
			instance.ReloadScripts();
		}
	}


	return 0;
}

extern "C" bool ModMenu(bool* show)
{
	if (show)
		instance.options.show_preferences = *show;
	else
		instance.options.show_preferences = !instance.options.show_preferences;
	return instance.options.show_preferences;
}

void OverlayInstance::SetTexture(ImTextureID texture)
{
	overlay_texture = texture;
	for (auto i = overlays.begin(); i != overlays.end(); ++i) {
		if (i->second->IsLoaded())
		{
			i->second->SetTexture(texture, &overlay_images);
		}
	}
}

OverlayInstance::OverlayInstance()
{
	options.show_preferences = true;
	style_colors = {
		{ "Text",                  &imgui_style.Colors[ImGuiCol_Text] },
		{ "TextDisabled",          &imgui_style.Colors[ImGuiCol_TextDisabled] },
		{ "WindowBg",              &imgui_style.Colors[ImGuiCol_WindowBg] },
		{ "ChildWindowBg",         &imgui_style.Colors[ImGuiCol_ChildWindowBg] },
		{ "PopupBg",               &imgui_style.Colors[ImGuiCol_PopupBg] },
		{ "Border",                &imgui_style.Colors[ImGuiCol_Border] },
		{ "BorderShadow",          &imgui_style.Colors[ImGuiCol_BorderShadow] },
		{ "FrameBg",               &imgui_style.Colors[ImGuiCol_FrameBg] },
		{ "FrameBgHovered",        &imgui_style.Colors[ImGuiCol_FrameBgHovered] },
		{ "FrameBgActive",         &imgui_style.Colors[ImGuiCol_FrameBgActive] },
		{ "TitleBg",               &imgui_style.Colors[ImGuiCol_TitleBg] },
		{ "TitleBgCollapsed",      &imgui_style.Colors[ImGuiCol_TitleBgCollapsed] },
		{ "TitleBgActive",         &imgui_style.Colors[ImGuiCol_TitleBgActive] },
		{ "MenuBarBg",             &imgui_style.Colors[ImGuiCol_MenuBarBg] },
		{ "ScrollbarBg",           &imgui_style.Colors[ImGuiCol_ScrollbarBg] },
		{ "ScrollbarGrab",         &imgui_style.Colors[ImGuiCol_ScrollbarGrab] },
		{ "ScrollbarGrabHovered",  &imgui_style.Colors[ImGuiCol_ScrollbarGrabHovered] },
		{ "ScrollbarGrabActive",   &imgui_style.Colors[ImGuiCol_ScrollbarGrabActive] },
		{ "ComboBg",               &imgui_style.Colors[ImGuiCol_ComboBg] },
		{ "CheckMark",             &imgui_style.Colors[ImGuiCol_CheckMark] },
		{ "SliderGrab",            &imgui_style.Colors[ImGuiCol_SliderGrab] },
		{ "SliderGrabActive",      &imgui_style.Colors[ImGuiCol_SliderGrabActive] },
		{ "Button",                &imgui_style.Colors[ImGuiCol_Button] },
		{ "ButtonHovered",         &imgui_style.Colors[ImGuiCol_ButtonHovered] },
		{ "ButtonActive",          &imgui_style.Colors[ImGuiCol_ButtonActive] },
		{ "Header",                &imgui_style.Colors[ImGuiCol_Header] },
		{ "HeaderHovered",         &imgui_style.Colors[ImGuiCol_HeaderHovered] },
		{ "HeaderActive",          &imgui_style.Colors[ImGuiCol_HeaderActive] },
		{ "Column",                &imgui_style.Colors[ImGuiCol_Column] },
		{ "ColumnHovered",         &imgui_style.Colors[ImGuiCol_ColumnHovered] },
		{ "ColumnActive",          &imgui_style.Colors[ImGuiCol_ColumnActive] },
		{ "ResizeGrip",            &imgui_style.Colors[ImGuiCol_ResizeGrip] },
		{ "ResizeGripHovered",     &imgui_style.Colors[ImGuiCol_ResizeGripHovered] },
		{ "ResizeGripActive",      &imgui_style.Colors[ImGuiCol_ResizeGripActive] },
		{ "CloseButton",           &imgui_style.Colors[ImGuiCol_CloseButton] },
		{ "CloseButtonHovered",    &imgui_style.Colors[ImGuiCol_CloseButtonHovered] },
		{ "CloseButtonActive",     &imgui_style.Colors[ImGuiCol_CloseButtonActive] },
		{ "PlotLines",             &imgui_style.Colors[ImGuiCol_PlotLines] },
		{ "PlotLinesHovered",      &imgui_style.Colors[ImGuiCol_PlotLinesHovered] },
		{ "PlotHistogram",         &imgui_style.Colors[ImGuiCol_PlotHistogram] },
		{ "PlotHistogramHovered",  &imgui_style.Colors[ImGuiCol_PlotHistogramHovered] },
		{ "TextSelectedBg",        &imgui_style.Colors[ImGuiCol_TextSelectedBg] },
		{ "ModalWindowDarkening",  &imgui_style.Colors[ImGuiCol_ModalWindowDarkening] },
	};
	preference_nodes = {
		//{ "Styles", {
		//	{ "GrabMinSize",               PreferenceNode::Type::Float, &imgui_style.GrabMinSize },
		//	{ "GrabRounding",              PreferenceNode::Type::Float, &imgui_style.GrabRounding },
		//	{ "WindowRounding",            PreferenceNode::Type::Float, &imgui_style.WindowRounding },
		//}
		//},
		{ "Colors",{
			{ "Text",                  PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Text] },
			{ "TextDisabled",          PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_TextDisabled] },
			{ "WindowBg",              PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_WindowBg] },
			{ "ChildWindowBg",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ChildWindowBg] },
			{ "PopupBg",               PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PopupBg] },
			{ "Border",                PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Border] },
			{ "BorderShadow",          PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_BorderShadow] },
			{ "FrameBg",               PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_FrameBg] },
			{ "FrameBgHovered",        PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_FrameBgHovered] },
			{ "FrameBgActive",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_FrameBgActive] },
			{ "TitleBg",               PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_TitleBg] },
			{ "TitleBgCollapsed",      PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_TitleBgCollapsed] },
			{ "TitleBgActive",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_TitleBgActive] },
			{ "MenuBarBg",             PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_MenuBarBg] },
			{ "ScrollbarBg",           PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ScrollbarBg] },
			{ "ScrollbarGrab",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ScrollbarGrab] },
			{ "ScrollbarGrabHovered",  PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ScrollbarGrabHovered] },
			{ "ScrollbarGrabActive",   PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ScrollbarGrabActive] },
			{ "ComboBg",               PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ComboBg] },
			{ "CheckMark",             PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_CheckMark] },
			{ "SliderGrab",            PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_SliderGrab] },
			{ "SliderGrabActive",      PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_SliderGrabActive] },
			{ "Button",                PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Button] },
			{ "ButtonHovered",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ButtonHovered] },
			{ "ButtonActive",          PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ButtonActive] },
			{ "Header",                PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Header] },
			{ "HeaderHovered",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_HeaderHovered] },
			{ "HeaderActive",          PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_HeaderActive] },
			{ "Column",                PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_Column] },
			{ "ColumnHovered",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ColumnHovered] },
			{ "ColumnActive",          PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ColumnActive] },
			{ "ResizeGrip",            PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ResizeGrip] },
			{ "ResizeGripHovered",     PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ResizeGripHovered] },
			{ "ResizeGripActive",      PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ResizeGripActive] },
			{ "CloseButton",           PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_CloseButton] },
			{ "CloseButtonHovered",    PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_CloseButtonHovered] },
			{ "CloseButtonActive",     PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_CloseButtonActive] },
			{ "PlotLines",             PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PlotLines] },
			{ "PlotLinesHovered",      PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PlotLinesHovered] },
			{ "PlotHistogram",         PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PlotHistogram] },
			{ "PlotHistogramHovered",  PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_PlotHistogramHovered] },
			{ "TextSelectedBg",        PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_TextSelectedBg] },
			{ "ModalWindowDarkening",  PreferenceNode::Type::Color4, &imgui_style.Colors[ImGuiCol_ModalWindowDarkening] },
		},
		}
	};


}

OverlayInstance::~OverlayInstance()
{
	overlays.clear();
}

void OverlayInstance::FontsPreferences() {
	if (ImGui::TreeNode("Fonts"))
	{
		ImGui::Text("The font settings are applied at the next start.");
		ImGui::Text("Default font is \'Default\' with fixed font size 13.0");
		{
			std::vector<const char*> data;
			data.reserve(fonts.size());
			for (int i = 0; i < fonts.size(); ++i)
			{
				data.push_back(fonts[i].fontname.c_str());
			}

			static int index_ = -1;
			static char buf[100] = { 0, };
			static int current_item = -1;
			static float font_size = 12.0f;
			static int glyph_range = 0;
			static int fontname_idx = -1;
			static int index = -1;
			bool decIndex = false;
			bool incIndex = false;
			if (ImGui::ListBox("Fonts", &index_, data.data(), data.size()))
			{
				index = index_;
				strcpy(buf, fonts[index].fontname.c_str());
				font_size = fonts[index].font_size;
				auto ri = std::find(glyph_range_key.begin(), glyph_range_key.end(), fonts[index].glyph_range);
				if (ri != glyph_range_key.end())
				{
					glyph_range = ri - glyph_range_key.begin();
				}
				std::string val = boost::to_lower_copy(fonts[index].fontname);
				auto fi = std::find_if(font_cstr_filenames.begin(), font_cstr_filenames.end(), [&val](const std::string& a) {
					return val == boost::to_lower_copy(a);
				});
				if (fi != font_cstr_filenames.end())
				{
					fontname_idx = fi - font_cstr_filenames.begin();
				}
				else
				{
					fontname_idx = -1;
				}
				font_setting_dirty = true;
			}
			if (ImGui::Button("Up"))
			{
				if (index > 0)
				{
					std::swap(fonts[index], fonts[index - 1]);
					Save();
					decIndex = true;
					font_setting_dirty = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Down"))
			{
				if (index + 1 < fonts.size())
				{
					std::swap(fonts[index], fonts[index + 1]);
					Save();
					incIndex = true;
					font_setting_dirty = true;
				}
			}
			ImGui::SameLine();
			//if (ImGui::Button("Edit"))
			//{
			//	if(index_ >= 0)
			//		ImGui::OpenPopup("Edit Column");
			//}
			//ImGui::SameLine();

			if (ImGui::Button("Remove"))
			{
				if (index >= 0)
				{
					fonts.erase(fonts.begin() + index);
					font_setting_dirty = true;
					Save();
				}
				if (index >= fonts.size())
				{
					decIndex = true;
				}
			}
			if (decIndex)
			{
				--index_;
				index = index_;
				if (index >= 0)
				{
					strcpy(buf, fonts[index].fontname.c_str());
					font_size = fonts[index].font_size;
					auto ri = std::find(glyph_range_key.begin(), glyph_range_key.end(), fonts[index].glyph_range);
					if (ri != glyph_range_key.end())
					{
						glyph_range = ri - glyph_range_key.begin();
					}
					font_setting_dirty = true;
				}
				else
				{
					strcpy(buf, "");
					font_size = 15.0f;
					glyph_range = 0;
				}
			}
			if (incIndex)
			{
				++index_;
				index = index_;
				if (index < fonts.size())
				{
					strcpy(buf, fonts[index].fontname.c_str());
					font_size = fonts[index].font_size;
					auto ri = std::find(glyph_range_key.begin(), glyph_range_key.end(), fonts[index].glyph_range);
					if (ri != glyph_range_key.end())
					{
						glyph_range = ri - glyph_range_key.begin();
					}
					font_setting_dirty = true;
				}
				else
				{
					strcpy(buf, "");
					font_size = 15.0f;
					glyph_range = 0;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Append"))
			{
				font_size = font_sizes;
				ImGui::OpenPopup("Append Column");
			}
			if (ImGui::BeginPopup("Append Column"))
			{
				static char buf[100] = { 0, };
				static int glyph_range = 0;
				static int fontname_idx = -1;
				if (ImGui::Combo("FontName", &fontname_idx, font_cstr_filenames.data(), font_cstr_filenames.size()))
				{
					if (fontname_idx >= 0)
					{
						strcpy(buf, font_cstr_filenames[fontname_idx]);
					}
				}
				if (ImGui::InputText("FontName", buf, 99))
				{
					std::string val = boost::to_lower_copy(std::string(buf));
					auto fi = std::find_if(font_cstr_filenames.begin(), font_cstr_filenames.end(), [&val](const std::string& a) {
						return val == boost::to_lower_copy(a);
					});
					if (fi != font_cstr_filenames.end())
					{
						fontname_idx = fi - font_cstr_filenames.begin();
					}
					else
					{
						fontname_idx = -1;
					}
					Save();
				}
				if (ImGui::Combo("GlyphRange", &glyph_range, glyph_range_key.data(), glyph_range_key.size()))
				{
				}
				if (ImGui::InputFloat("Size", &font_size, 0.5f))
				{
					font_size = std::min(std::max(font_size, 6.0f), 30.0f);
				}
				if (ImGui::Button("Append"))
				{
					if (glyph_range >= 0)
					{
						Font font;
						font.fontname = buf;
						font.font_size = font_size;
						font.glyph_range = glyph_range_key[glyph_range];
						fonts.push_back(font);
						ImGui::CloseCurrentPopup();
						strcpy(buf, "");
						current_item = -1;
						font_size = font_sizes;
						font_setting_dirty = true;
						Save();
					}
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Default"))
			{
				font_sizes = 13;
				fonts = {
					Font("consolab.ttf", "Default", font_sizes),
					Font("Default", "Default", font_sizes), // Default will ignore font size.
					Font("ArialUni.ttf", "Japanese", font_sizes),
					Font("NanumBarunGothic.ttf", "Korean", font_sizes),
					Font("gulim.ttc", "Korean", font_sizes),
				};
				font_setting_dirty = true;
			}

			//if (ImGui::BeginPopup("Edit Column"))
			if (index >= 0)
			{
				ImGui::BeginGroup();
				ImGui::Text("Edit");
				ImGui::Separator();
				//font_cstr_filenames;
				if (ImGui::Combo("FontName", &fontname_idx, font_cstr_filenames.data(), font_cstr_filenames.size()))
				{
					if (fontname_idx >= 0)
					{
						fonts[index].fontname = font_cstr_filenames[fontname_idx];
						strcpy(buf, font_cstr_filenames[fontname_idx]);
						Save();
						font_setting_dirty = true;
					}
				}
				if (ImGui::InputText("FontName", buf, 99))
				{
					fonts[index].fontname = buf;
					std::string val = boost::to_lower_copy(fonts[index].fontname);
					auto fi = std::find_if(font_cstr_filenames.begin(), font_cstr_filenames.end(), [&val](const std::string& a) {
						return val == boost::to_lower_copy(a);
					});
					if (fi != font_cstr_filenames.end())
					{
						fontname_idx = fi - font_cstr_filenames.begin();
					}
					else
					{
						fontname_idx = -1;
					}
					font_setting_dirty = true;
					Save();
				}
				if (ImGui::Combo("GlyphRange", &glyph_range, glyph_range_key.data(), glyph_range_key.size()))
				{
					if (glyph_range >= 0)
					{
						fonts[index].glyph_range = glyph_range_key[glyph_range];
						font_setting_dirty = true;
						Save();
					}
				}
				if (ImGui::InputFloat("Size", &font_size, 0.5f))
				{
					font_size = std::min(std::max(font_size, 6.0f), 30.0f);
					fonts[index].font_size = font_size;
					font_setting_dirty = true;
					Save();
				}
				ImGui::Separator();
				ImGui::EndGroup();
			}
			if (ImGui::InputFloat("FontSizes (for all)", &font_sizes, 0.5f))
			{
				font_sizes = std::min(std::max(font_sizes, 6.0f), 30.0f);
				for (auto i = fonts.begin(); i != fonts.end(); ++i)
				{
					i->font_size = font_sizes;
				}
				font_size = font_sizes;
				font_setting_dirty = true;
				Save();
			}
		}
		ImGui::TreePop();
	}
}

void OverlayInstance::Preferences() {
	char buf[256] = { 0, };
	sprintf_s(buf, 256, "Preferences");
	options.show_preferences = true;
	if (ImGui::Begin(buf, &options.show_preferences, options.windows_default_sizes["Preferences"], -1, ImGuiWindowFlags_NoCollapse))
	{
		ImGui::Text("ACTWebSocketOverlay - %s", VERSION_LONG_STRING);
		ImGui::Separator();
		options.windows_default_sizes["Preferences"] = ImGui::GetWindowSize();
		if (ImGui::TreeNodeEx("Global", ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen))
		{
			FontsPreferences();
			for (auto i = preference_nodes.begin(); i != preference_nodes.end(); ++i)
			{
				PreferenceBase::Preferences(*i, preference_storage);
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Overlays", ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginGroup();
			ImGui::Text("Visible");
			for (auto i = overlays.begin(); i != overlays.end(); ++i) {
				if (ImGui::Checkbox((std::string("Show ") + i->second->name).c_str(), &i->second->IsVisible()))
				{
					Save();
				}
			}
			ImGui::Separator();
			ImGui::Text("more...");
			for (auto i = overlays.begin(); i != overlays.end(); ++i) {
				if (i->second->IsLoaded())
				{
					ImGui::BeginGroup();
					i->second->Preferences();
					ImGui::EndGroup();
				}
			}
			ImGui::EndGroup();
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Reload"))
		{
			if (ImGui::Button("Reload Overlays"))
			{
				instance.ReloadScripts();
			}
			ImGui::TreePop();
		}
		ImGui::Separator();
		if (ImGui::TreeNode("About"))
		{
			ImGui::Text("Version : %s", VERSION_LONG_STRING);
			ImGui::Text("Github : https://github.com/ZCube/ACTWebSocket");
			ImGui::Text("Github : https://github.com/ZCube/ACTWebSocketOverlay");
			ImGui::Text("");
			ImGui::Text("Path : %s", (root_path).string().c_str());
			ImGui::Text("Script Search : %s", (root_path / "*" / "script.json").string().c_str());
			ImGui::Text("");
			if (ImGui::TreeNode("Libraries"))
			{
				if (ImGui::TreeNode("jsoncpp"))
				{
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("lua"))
				{
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("boost"))
				{
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("beast"))
				{
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("zlib"))
				{
					ImGui::TreePop();
				}
#if defined(USE_SSL)
				if (ImGui::TreeNode("openssl"))
				{
					ImGui::TreePop();
				}
#endif
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		ImGui::End();

	}
}

void OverlayInstance::Render(ImGuiContext * context)
{
	bool move_key_pressed = GetAsyncKeyState(VK_SHIFT) && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_MENU);
	bool use_input = options.movable || options.show_preferences || move_key_pressed;

	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiStyle backup = style;

	bool isVisible = false;
	for (auto i = overlays.begin(); i != overlays.end(); ++i) {
		style = imgui_style;
		current_storage = &i->second->preference_storage;
		if (i->second->IsVisible())
		{
			i->second->Render(use_input, &options);
			isVisible = true;
		}
	}
	current_storage = nullptr;
	style = imgui_style;
	if (options.show_preferences)
	{
		Preferences();
	}

	{
		auto& io = ImGui::GetIO();
		if (!use_input)
		{
			io.WantCaptureMouse = false;
		}
		if (!options.show_preferences)
		{
			io.WantCaptureKeyboard = false;
		}
	}

	style = backup;
}

void OverlayInstance::Load() {
	Json::Reader r;
	Json::Value setting;
	try {
		std::ifstream fin(setting_path.wstring());
		if (r.parse(fin, setting))
		{
			preference_storage.FromJson(setting);

			{
				auto it = preference_storage.boolean_map.find("Movable");
				if (it != preference_storage.boolean_map.end())
				{
					options.movable = it->second;
				}
			}

			int count = 0;
			int transparent_count = 0;
			for (auto i = style_colors.begin(); i != style_colors.end(); ++i)
			{
				auto j = preference_storage.color_map.find(i->first);
				if (j != preference_storage.color_map.end())
				{
					if (j->second.w == 0)
						++transparent_count;
					++count;
				}
			}
			// load to style

			if (transparent_count != count)
			{
				for (auto i = style_colors.begin(); i != style_colors.end(); ++i)
				{
					auto j = preference_storage.color_map.find(i->first);
					if (j != preference_storage.color_map.end())
					{
						*i->second = j->second;
					}
				}
			}

			// global
			Json::Value fonts = setting.get("fonts", Json::nullValue);
			std::string fonts_str = "fonts";
			if (setting.find(&*fonts_str.begin(), &*fonts_str.begin() + fonts_str.size()) != nullptr)
			{
				if (fonts.size() > 0)
				{
					this->fonts.clear();
					for (auto i = fonts.begin();
						i != fonts.end();
						++i)
					{
						Font font;
						font.FromJson(*i);
						this->fonts.push_back(font);
					}
				}
			}
			// global
			Json::Value windows = setting.get("windows", Json::nullValue);
			std::string windows_str = "windows";
			if (setting.find(&*windows_str.begin(), &*windows_str.begin() + windows_str.size()) != nullptr)
			{
				if (windows.size() > 0)
				{
					Json::Value;
					for (auto i = windows.begin();
						i != windows.end();
						++i)
					{
						Json::Value& win = *i;
						std::string name = win["name"].asString();
						if (name.empty())
							continue;
						ImVec2 pos = ImVec2(win["x"].asFloat(), win["y"].asFloat());
						ImVec2 size = ImVec2(win["width"].asFloat(), win["height"].asFloat());
						options.windows_default_sizes[name] = size;
						ImGuiContext & g = *ImGui::GetCurrentContext();
						g.Initialized = true;
						size = ImMax(size, g.Style.WindowMinSize);
						ImGuiIniData* settings = nullptr;
						ImGuiID id = ImHash(name.c_str(), 0);
						{
							for (int i = 0; i != g.Settings.Size; i++)
							{
								ImGuiIniData* ini = &g.Settings[i];
								if (ini->Id == id)
								{
									settings = ini;
									break;
								}
							}
							if (settings == nullptr)
							{
								GImGui->Settings.resize(GImGui->Settings.Size + 1);
								ImGuiIniData* ini = &GImGui->Settings.back();
								ini->Name = ImStrdup(name.c_str());
								ini->Id = ImHash(name.c_str(), 0);
								ini->Collapsed = false;
								ini->Pos = ImVec2(FLT_MAX, FLT_MAX);
								ini->Size = ImVec2(0, 0);
								settings = ini;
							}
						}
						if (settings)
						{
							settings->Pos = pos;
							settings->Size = size;
						}
					}
				}
			}
		}
		for (auto i = overlays.begin(); i != overlays.end(); ++i) {
			i->second->FromJson(setting);
		}

		fin.close();
	}
	catch (...)
	{
	}
}

void OverlayInstance::Save() {
	Json::StyledWriter w;
	Json::Value setting;
	Json::Value color;

	// save from style
	for (auto i = style_colors.begin(); i != style_colors.end(); ++i)
	{
		preference_storage.color_map[i->first] = *i->second;
	}

	preference_storage.boolean_map["Movable"] = options.movable;

	preference_storage.ToJson(setting);

	Json::Value fonts;
	for (int i = 0; i < this->fonts.size(); ++i)
	{
		Json::Value font;
		this->fonts[i].ToJson(font);
		fonts.append(font);
	}
	setting["fonts"] = fonts;

	Json::Value windows;
	{
		ImGuiContext& g = *ImGui::GetCurrentContext();
		{
			for (int i = 0; i != g.Windows.Size; i++)
			{
				ImGuiWindow* window = g.Windows[i];
				if (window->Flags & ImGuiWindowFlags_NoSavedSettings)
					continue;
				Json::Value win;
				win["name"] = window->Name;
				win["x"] = window->Pos.x;
				win["y"] = window->Pos.y;
				win["width"] = window->SizeFull.x;
				win["height"] = window->SizeFull.y;
				windows[window->Name] = win;
			}
		}
	}
	setting["windows"] = windows;

	for (auto i = overlays.begin(); i != overlays.end(); ++i) {
		i->second->ToJson(setting);
	}
	std::ofstream fout(setting_path.wstring());
	fout << w.write(setting);
	fout.close();
}


void OverlayInstance::Init(ImGuiContext * context, const boost::filesystem::path & path) {
	root_path = path;
	setting_path = root_path / "mod.json";

	if (!initialized)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		imgui_style = style;
		Load();
		LoadFonts();
		boost::mutex::scoped_lock l(m);
		LoadOverlays();
		Load();
		initialized = true;
	}
	else
	{
		ReloadScripts();
	}
	LoadTexture();
}

void OverlayInstance::ReloadScripts() {
	overlays.clear();
	ClearUniqueValues();
	LoadOverlays();
	SetTexture(overlay_texture);
	Load();
}

void OverlayInstance::UnloadScripts() {
	overlays.clear();
	ClearUniqueValues();
}

void OverlayInstance::LoadTexture()
{
	// texture
	boost::filesystem::path texture_path = root_path / "overlay_atlas.png";
	boost::filesystem::path atlas_json_path = root_path / "overlay_atlas.json";

	if (!overlay_texture_filedata)
	{
		if (boost::filesystem::exists(texture_path))
		{
			FILE *file;
			bool success = false;

			if (_wfopen_s(&file, texture_path.wstring().c_str(), L"rb") == 0)
			{
				overlay_texture_filedata = stbi_load_from_file(file, &overlay_texture_width, &overlay_texture_height, &overlay_texture_channels, STBI_rgb_alpha);
				fclose(file);
			}
		}
	}
	if (boost::filesystem::exists(atlas_json_path) && overlay_texture_width > 0 && overlay_texture_height > 0)
	{
		bool success = false;

		std::ifstream fin(atlas_json_path.wstring().c_str());
		if (fin.is_open())
		{
			std::unordered_map<std::string, Image> overlay_images;
			Json::Value overlay_atlas;;
			Json::Reader r;
			if (r.parse(fin, overlay_atlas))
			{
				for (auto i = overlay_atlas.begin(); i != overlay_atlas.end(); ++i)
				{
					std::string name = boost::to_lower_copy(i.key().asString());
					boost::replace_all(name, ".png", "");
					Image im;
					im.x = (*i)["left"].asInt();
					im.y = (*i)["top"].asInt();
					im.width = (*i)["width"].asInt();
					im.height = (*i)["height"].asInt();
					im.uv0 = ImVec2(((float)im.x + 0.5f) / (float)overlay_texture_width,
						((float)im.y + 0.5f) / (float)overlay_texture_width);
					im.uv1 = ImVec2((float)(im.x + im.width - 1 + 0.5f) / (float)overlay_texture_width,
						(float)(im.y + im.height - 1 + 0.5f) / (float)overlay_texture_width);
					overlay_images[name] = im;
				}
			}
			this->overlay_images = overlay_images;
		}
	}
}

void OverlayInstance::LoadOverlays()
{
	{
		auto awio = overlays["AWIO"] = std::make_shared<AWIOOverlay>();
		awio->SetRoot(this);
		if (awio->Init(root_path))
		{
			awio->WebSocketRun();
		}
	}
	for (boost::filesystem::directory_iterator itr(root_path); itr != boost::filesystem::directory_iterator(); ++itr) {
		if (boost::filesystem::is_directory(*itr))
		{
			std::string u8name = STR2UTF8((itr)->path().filename().string().c_str());
			auto script_path = (*itr) / "script.json";

			if (boost::filesystem::exists(script_path))
			{
				Json::Value value;
				Json::Reader reader;
				std::ifstream fin(script_path.wstring().c_str());
				if (reader.parse(fin, value))
				{
					if (value["Type"].asString() == "Lua")
					{
						auto& overlay = overlays[u8name] = std::make_shared<OverlayContextLua>();
						overlay->SetRoot(this);
						if (overlay->Init((*itr)))
						{
							overlay->WebSocketRun();
						}
					}
				}
			}
		}
	}
}

void OverlayInstance::LoadFonts()
{
	// Changing or adding fonts at runtime causes a crash.
	// TODO: is it possible ?...

	ImGuiIO& io = ImGui::GetIO();

	WCHAR result[MAX_PATH] = {};
	GetWindowsDirectoryW(result, MAX_PATH);
	boost::filesystem::path windows_dir = result;

	// font
	ImGui::GetIO().Fonts->Clear();

	std::map<std::string, const ImWchar*> glyph_range_map = {
		{ "Default", io.Fonts->GetGlyphRangesDefault() },
		{ "Chinese", io.Fonts->GetGlyphRangesChinese() },
		{ "Cyrillic", io.Fonts->GetGlyphRangesCyrillic() },
		{ "Japanese", io.Fonts->GetGlyphRangesJapanese() },
		{ "Korean", io.Fonts->GetGlyphRangesKorean() },
		{ "Thai", io.Fonts->GetGlyphRangesThai() },
	};

	// Add fonts in this order.
	glyph_range_key = {
		"Default",
		"Chinese",
		"Cyrillic",
		"Japanese",
		"Korean",
		"Thai",
	};

	std::vector<boost::filesystem::path> font_find_folders = {
		root_path,
		root_path / "Fonts",
		windows_dir / "Fonts", // windows fonts
	};

	ImFontConfig config;
	config.MergeMode = false;


	font_paths.clear();
	font_filenames.clear();
	boost::filesystem::directory_iterator itr, end_itr;

	font_filenames.push_back("Default");
	for (auto i = font_find_folders.begin(); i != font_find_folders.end(); ++i)
	{
		if (boost::filesystem::exists(*i))
		{
			for (boost::filesystem::directory_iterator itr(*i); itr != end_itr; ++itr)
			{
				if (is_regular_file(itr->path())) {
					std::string extension = boost::to_lower_copy(itr->path().extension().string());
					if (extension == ".ttc" || extension == ".ttf")
					{
						font_paths.push_back(itr->path());
						font_filenames.push_back(itr->path().filename().string());
					}
				}
			}
		}
	}
	{
		for (auto i = font_filenames.begin(); i != font_filenames.end(); ++i)
		{
			font_cstr_filenames.push_back(i->c_str());
		}
	}

	for (auto i = glyph_range_key.begin(); i != glyph_range_key.end(); ++i)
	{
		bool is_loaded = false;

		for (auto j = fonts.begin(); j != fonts.end() && !is_loaded; ++j)
		{
			if (j->glyph_range == *i)
			{
				if (j->fontname == "Default")
				{
					io.Fonts->AddFontDefault(&config);
					is_loaded = true;
					config.MergeMode = true;
				}
				else
				{
					for (auto k = font_find_folders.begin(); k != font_find_folders.end(); ++k)
					{
						// ttf, ttc only
						auto fontpath = (*k) / j->fontname;
						if (boost::filesystem::exists(fontpath))
						{
							io.Fonts->AddFontFromFileTTF((fontpath).string().c_str(), j->font_size, &config, glyph_range_map[j->glyph_range]);
							is_loaded = true;
							config.MergeMode = true;
						}
					}
				}
			}
		}

		if (*i == "Default" && !is_loaded)
		{
			io.Fonts->AddFontDefault(&config);
			is_loaded = true;
			config.MergeMode = true;
		}
	}
	// do not remove
	io.Fonts->AddFontDefault();
	if (fonts.empty())
	{
		font_sizes = 13;
	}
	else
	{
		font_sizes = fonts[0].font_size;
		for (auto i = fonts.begin(); i != fonts.end(); ++i)
		{
			font_sizes = std::min(font_sizes, i->font_size);
		}
	}
	font_setting_dirty = false;
}
