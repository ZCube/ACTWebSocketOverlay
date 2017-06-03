/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include "Table.h"
#include <unordered_map>
#include <boost/algorithm/string.hpp>

ImVec4 ColorWithAlpha(ImVec4 col, float alpha)
{
	col.w = alpha;
	return col;
}

void Table::RenderTableColumnHeader(int height, ImTextureID table_texture, const std::unordered_map<std::string, Image>* table_images)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	const ImGuiIO& io = ImGui::GetIO();
	int base = ImGui::GetCursorPosY();
	for (int i = 0; i < columns.size(); ++i)
	{
		if (!columns[i].visible)
			continue;
		const ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 winpos = ImGui::GetWindowPos();
		ImVec2 pos = ImGui::GetCursorPos();
		pos = window->DC.CursorPos;

		{
			ImGui::SetCursorPos(ImVec2(columns[i].offset + style.ItemInnerSpacing.x, base));
			ImVec2 winpos = ImGui::GetWindowPos();
			ImVec2 pos = ImGui::GetCursorPos();
			pos = window->DC.CursorPos;
			ImRect clip, align;

			std::string text = columns[i].Title;
			if (i == 1)
			{
				if (ImGui::Button("Name")) show_name = !show_name;
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(ImVec4(0, 0, 0, 1), text_opacity * global_opacity));
				ImGui::RenderTextClipped(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + (columns[i].size + 1) * io.FontGlobalScale, pos.y + height + 1),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					ImVec2(0.5f, 0.5f),
					//columns[i].align,
					nullptr);
				ImGui::PopStyleColor();
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(graph_text_color, text_opacity * global_opacity));
				ImGui::RenderTextClipped(pos, ImVec2(pos.x + (columns[i].size) * io.FontGlobalScale, pos.y + height),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					ImVec2(0.5f, 0.5f),
					//columns[i].align,
					nullptr);
				ImGui::PopStyleColor();
			}
		}
		ImGui::NewLine();
	}
	ImGui::SetCursorPos(ImVec2(0, base + height));
}

void Table::RenderTableRow(int row, int height, ImTextureID table_texture, const std::unordered_map<std::string, Image>* table_images)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	const ImGuiIO& io = ImGui::GetIO();
	int offset = 0;
	int base = ImGui::GetCursorPosY();
	ImGui::SetCursorPos(ImVec2(0, base));

	int i = row;
	{
		std::string& jobStr = values[i][0];
		const std::string& progressStr = values[i][2];
		std::string& nameStr = values[i][1];

		if (maxValue == 0.0)
			maxValue = 1.0;
		float progress = atof(progressStr.c_str()) / maxValue;
		ImVec4 progressColor = rows[i].color != nullptr ? *rows[i].color : etc_color;

		progressColor.w = graph_opacity * global_opacity;
		const ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 winpos = ImGui::GetWindowPos();
		ImVec2 pos = ImGui::GetCursorPos();
		pos = window->DC.CursorPos;

		if(table_images && table_texture)
		{
			int length = (ImGui::GetWindowSize().x) * progress;
			ImRect bb(ImVec2(pos.x, pos.y), ImVec2(pos.x + length, pos.y + height));

			auto graph_image = table_images->find(graph);
			if (graph_image != table_images->end())
			{
				window->DrawList->AddImage(table_texture, bb.Min, ImVec2(bb.Max.x, pos.y + height),
					graph_image->second.uv0, graph_image->second.uv1, ImGui::GetColorU32(progressColor));
			}
			else
			{
				ImGui::RenderFrame(bb.Min, ImVec2(bb.Max.x, pos.y + height), ImGui::GetColorU32(progressColor), true, style.FrameRounding);
			}
		}
		//ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(0.5, 0.5, 0.5, 0.0)), true, style.FrameRounding);
		//ImRect bb2(pos, ImVec2(pos.x + ImGui::GetWindowSize().x * progress, pos.y + height));
		//ImGui::RenderFrame(bb2.Min, bb2.Max, ImGui::GetColorU32(progressColor), true, style.FrameRounding);
		for (int j = 0; j < columns.size() && j < values[i].size(); ++j)
		{
			if (!columns[j].visible)
				continue;
			ImGui::SetCursorPos(ImVec2(columns[j].offset + style.ItemInnerSpacing.x, base));
			ImVec2 winpos = ImGui::GetWindowPos();
			ImVec2 pos = ImGui::GetCursorPos();
			pos = window->DC.CursorPos;
			ImRect clip, align;

			std::string text = values[i][j];
			if (j == 0 && table_texture)
			{
				std::string icon = boost::to_lower_copy(text);
				std::unordered_map<std::string, Image>::iterator im;
				if (table_images)
				{
					auto im = table_images->find(icon);
					if (im != table_images->end())
					{
						ImGuiWindow* window = ImGui::GetCurrentWindow();
						window->DrawList->AddImage(table_texture, ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + (columns[j].size + 1)*io.FontGlobalScale, pos.y + height + 1),
							im->second.uv0, im->second.uv1);// , GetColorU32(tint_col));
					}
				}
			}
			else if (j != 1 || show_name)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(ImVec4(0, 0, 0, 1), text_opacity * global_opacity));
				ImGui::RenderTextClipped(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + (columns[j].size + 1) * io.FontGlobalScale, pos.y + height + 1),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					columns[j].align,
					nullptr);
				ImGui::PopStyleColor();
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(graph_text_color, text_opacity * global_opacity));
				ImGui::RenderTextClipped(pos, ImVec2(pos.x + columns[j].size * io.FontGlobalScale, pos.y + height),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					columns[j].align,
					nullptr);
				ImGui::PopStyleColor();
			}
		}
		{
			ImVec2 pos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + height));
			ImGui::NewLine();
		}
	}
	ImGui::SetCursorPos(ImVec2(0, base + height));
}

void Table::RenderTable(ImTextureID table_texture, const std::unordered_map<std::string, Image>* table_images)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	const ImGuiIO& io = ImGui::GetIO();
	int windowWidth = ImGui::GetWindowContentRegionWidth();
	int column_max = columns.size();
	const int height = 20 * io.FontGlobalScale;
	UpdateColumnWidth(windowWidth, height, column_max, io.FontGlobalScale);

	ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(graph_text_color, text_opacity * global_opacity));
	RenderTableColumnHeader(height, table_texture, table_images);

	ImGui::Separator();

	for (int row = 0; row < values.size(); ++row)
	{
		RenderTableRow(row, height, table_texture, table_images);
	}
	ImGui::PopStyleColor(1);
}
