/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include <imgui.h>
#include <string>
#include <boost/algorithm/hex.hpp>
#include "Serializable.h"
#include <imgui_internal.h>

ImVec4 htmlCodeToImVec4(const std::string hex)
{
	unsigned char dat[256] = { 0, };
	boost::algorithm::unhex(hex, dat);
	//if (hex.length() < 8)
	//	dat[3] = 0;
	return ImVec4((float)dat[0] / 255.0f, (float)dat[1] / 255.0f, (float)dat[2] / 255.0f, (float)dat[3] / 255.0f);
}

const std::string ImVec4TohtmlCode(ImVec4 val)
{
	unsigned char dat[256] = { 0, };
	dat[0] = (unsigned char)std::min(255, std::max(0, (int)(val.x * 255.0f)));
	dat[1] = (unsigned char)std::min(255, std::max(0, (int)(val.y * 255.0f)));
	dat[2] = (unsigned char)std::min(255, std::max(0, (int)(val.z * 255.0f)));
	dat[3] = (unsigned char)std::min(255, std::max(0, (int)(val.w * 255.0f)));
	std::string ret;
	boost::algorithm::hex(dat, dat + 4, std::back_inserter(ret));
	return ret;
}

void OverlayOption::SaveWindowPos()
{
	if (ImGui::GetCurrentContext())
	{
		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();
		if (auto window = ImGui::GetCurrentWindow())
		{
			windows_default_pos[window->Name] = pos;
			//ImVec2(
			//	pos.x / ImGui::GetIO().DisplaySize.x,
			//	pos.y / ImGui::GetIO().DisplaySize.y);
			windows_default_sizes[window->Name] = size;
			//ImVec2(
			//	size.x / ImGui::GetIO().DisplaySize.x,
			//	size.y / ImGui::GetIO().DisplaySize.y);
		}
	}
}

