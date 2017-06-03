/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#pragma once
#include <algorithm>
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <unordered_map>
#include <string>
#include "Serializable.h"

class Table
{
public:
	class Row
	{
	public:
		ImVec4* color = nullptr;
	};
	class Column : public Serializable
	{
	public:
		Column(std::string title_ = "", std::string index_ = "", int size_ = 0, int sizeWeight_ = 0, ImVec2 align_ = ImVec2(0.5f, 0.5f), bool visible_ = true)
			: Title(title_)
			, index(index_)
			, size(size_)
			, sizeWeight(sizeWeight_)
			, align(align_)
			, visible(visible_)
		{

		}
		void ToJson(Json::Value& value) const
		{
			value["width"] = size;
			value["title"] = Title;
			value["index"] = index;
			value["align"] = align.x;
		}
		void FromJson(Json::Value& value)
		{
			size = value["width"].asInt();
			Title = value["title"].asString();
			index = value["index"].asString();
			align.x = value["align"].asFloat();
		}
		int size = 0;
		int sizeWeight = 0;
		std::string Title = "";
		std::string index = "";
		bool visible = true;
		ImVec2 align = ImVec2(0.5f, 0.5f);
		int offset = 0;
	};

	int column_margin = 2;

	void UpdateColumnWidth(int width, int height, int column_max, float scale)
	{
		if (columns.size() > 0)
			columns[0].size = height / scale;
		int columnSizeWeightSum = 0;
		int columnSizeFixed = 0;
		for (int i = 0; i < column_max; ++i) {
			if (!columns[i].visible)
				continue;
			if (columns[i].sizeWeight != 0)
			{
				columnSizeWeightSum += columns[i].sizeWeight;
			}
			else
			{
				// icon size must be fixed.
				columnSizeFixed += (columns[i].size + column_margin * 2) * scale;
			}
		}
		int offset = 0;
		for (int i = 0; i < column_max; ++i) {
			if (!columns[i].visible)
				continue;
			if (columns[i].sizeWeight != 0)
			{
				columns[i].size = std::max(0, ((width - columnSizeFixed) * columns[i].sizeWeight) / columnSizeWeightSum) / scale;
			}
			columns[i].offset = offset + (column_margin)* scale;
			offset += (columns[i].size + column_margin) * scale;
		}

		for (int i = 2; i < column_max; ++i) {
			if (!columns[i].visible)
				continue;
			if (columns[i - 1].offset + columns[i - 1].size * scale > columns[i].offset)
			{
				columns[i].offset = columns[i - 1].offset + (columns[i - 1].size) * scale;
			}
		}
	}

	std::vector<int> offsets;
	std::vector<Column> columns;
	std::vector<Row> rows;
	std::vector<std::vector<std::string> > values;

	int progressColumn = 2;
	float maxValue = 0;
	bool show_name = true;

	Table(
		float& global_opacity_,
		float& text_opacity_,
		float& graph_opacity_,
		ImVec4& graph_text_color_,
		ImVec4& etc_color_,
		const std::string& graph_
	)
	: global_opacity(global_opacity_)
	, text_opacity(text_opacity_)
	, graph_opacity(graph_opacity_)
	, graph_text_color(graph_text_color_)
	, etc_color(etc_color_)
	, graph(graph_)
	{

	}

	float& global_opacity;
	float& text_opacity;
	float& graph_opacity;
	ImVec4& graph_text_color;
	ImVec4& etc_color;
	const std::string& graph;
	;

public:
	void RenderTableColumnHeader(int height, ImTextureID table_texture, const std::unordered_map<std::string, Image>* table_images);
	void RenderTableRow(int row, int height, ImTextureID table_texture, const std::unordered_map<std::string, Image>* table_images);
	void RenderTable(ImTextureID table_texture, const std::unordered_map<std::string, Image>* table_images);

};


ImVec4 ColorWithAlpha(ImVec4 col, float alpha);