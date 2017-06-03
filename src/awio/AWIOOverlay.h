/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/
#pragma once
#include "OverlayContext.h"
#include <json/json.h>
#include "Table.h"
#include <boost/algorithm/string.hpp>

class AWIOOverlay : public OverlayContextBase
{
protected:
	ImTextureID texture;
	Json::Value processed_json;

	std::string Title;
	std::string zone;
	std::string duration;
	std::string rdps;
	std::string rhps;

	static std::map<std::string, std::string> name_to_job_map;
	static std::vector<const char*> combatant_attribute_names;
	static std::map<std::string, std::vector<std::string>> color_category_map;
	static std::string default_pet_job;
	const std::unordered_map<std::string, Image>* images = nullptr;

	float& global_opacity = preference_storage.float_map["Global"] = 1.0f;
	float& text_opacity = preference_storage.float_map["Text"] = 1.0f;
	float& background_opacity = preference_storage.float_map["Background"] = 0.2f;
	float& graph_opacity = preference_storage.float_map["Graph"] = 0.4f;
	std::string graph = "center";

	Table dealerTable= Table(
		global_opacity,
		text_opacity,
		graph_opacity,
		preference_storage.color_map["GraphText"],
		preference_storage.color_map["etc"],
		graph
	);

	std::string your_name;

	virtual void Process(const std::string& message_str);
public:
	AWIOOverlay();
	virtual ~AWIOOverlay() 
	{
		WebSocketStop();
	}
	virtual bool Init(boost::filesystem::path path) {
		// dps 255 40 51
		// etc 0 0 0
		// healer 53 210 88
		// tank 47 96 255
		// ui 255 255 255
		std::vector<std::pair<std::string, std::string> > default_colors = {
			{"DPS", "FF2833"},
			{"Etc", "FFD500"},
			{"Healer", "35D258"},
			{"Tank", "2F60FF"},
			{"UI", "FFFFFF"} 
		};
		for (auto i = default_colors.begin(); i != default_colors.end(); ++i) {
			for (auto k = color_category_map[i->first].begin();
				k != color_category_map[i->first].end();
				++k)
			{
				preference_storage.color_map[*k] = preference_storage.color_map[i->first] = htmlCodeToImVec4(i->second);
			}
		}

		return true;
	};
	virtual void Render(bool use_input, struct OverlayOption* options)
	{
		try {
			ImVec2 padding(0, 0);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorWithAlpha(preference_storage.color_map["Background"], background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5, 0.5, 0.5, background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3, 0.3, 0.3, background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0, 0.0, 0.0, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(preference_storage.color_map["TitleText"], text_opacity * global_opacity));
			ImGui::Begin("AWIO", nullptr, options->windows_default_sizes["AWIO"], -1,
				ImGuiWindowFlags_NoTitleBar | (use_input ? NULL : ImGuiWindowFlags_NoInputs));
			options->windows_default_sizes["AWIO"] = ImGui::GetWindowSize();

			processed_data_mutex.lock();

			auto &io = ImGui::GetIO();
			if (images)
			{
				auto cog_img = images->find("cog");
				auto resize_img = images->find("resize");
				if (cog_img != images->end() && resize_img != images->end())
				{
					ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0,0,0,0));
					float icon_color_change = (cosf(GetTickCount() / 500.0f) + 1.0f) / 2.0f;
					ImVec4 color = ColorWithAlpha(preference_storage.color_map["TitleText"], text_opacity * global_opacity);
					color.x *= icon_color_change;
					color.y *= icon_color_change;
					color.z *= icon_color_change;
					auto p = ImGui::GetCursorPos();
					ImGui::BeginChild("Btn",
						//ImVec2(100,100),
						ImVec2((cog_img->second.width + resize_img->second.width + 36) * io.FontGlobalScale / 6, (cog_img->second.height + 18) * io.FontGlobalScale / 6),
						false,
						ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
					if (ImGui::ImageButton(texture, ImVec2(resize_img->second.width * io.FontGlobalScale / 6, resize_img->second.height * io.FontGlobalScale / 6), resize_img->second.uv0, resize_img->second.uv1, 2, ImVec4(0, 0, 0, 0), color))
					{
						options->movable = !options->movable;
					}
					ImGui::EndChild();
					ImGui::SameLine((cog_img->second.width + 18) * io.FontGlobalScale / 6);
					ImGui::BeginChild("Btn1",
						//ImVec2(100,100),
						ImVec2((cog_img->second.width + resize_img->second.width + 36) * io.FontGlobalScale / 6, (cog_img->second.height + 18) * io.FontGlobalScale / 6),
						false,
						ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
					if (ImGui::ImageButton(texture, ImVec2(cog_img->second.width * io.FontGlobalScale / 6, cog_img->second.height * io.FontGlobalScale / 6), cog_img->second.uv0, cog_img->second.uv1, 2, ImVec4(0, 0, 0, 0), color))
					{
						options->show_preferences = !options->show_preferences;
					}
					ImGui::EndChild();
					ImGui::SameLine();
					ImGui::SetCursorPos(p);
					ImGui::PopStyleColor(1);
				}
			}

			dealerTable.RenderTable(texture, images);

			ImGui::Separator();

			ImGui::End();
			ImGui::PopStyleVar(1);
			ImGui::PopStyleColor(5);

			typedef std::chrono::duration<long, std::chrono::milliseconds> milliseconds;
			static std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

			// auto save per 60 sec
			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() > 60000) // 60sec
			{
				//boost::unique_lock<boost::mutex> l(font_mutex);
				Save();
				start = end;
			}

			processed_data_mutex.unlock();
		}
		catch (std::exception& e)
		{
			ImGui::End();
			std::cerr << e.what() << std::endl;
		}
	};
	virtual bool IsLoaded()
	{
		return true;
	}
	virtual void Save()
	{
		//
		// prepare to save
		//
		PreferenceBase::Save();
	}
	virtual void Load()
	{
		PreferenceBase::Load();
		//
		// load 
		//
	}
	virtual void ToJson(Json::Value& setting) const {
		Json::Value dealer_columns;
		for (int i = 3; i < dealerTable.columns.size(); ++i)
		{
			Json::Value col;
			dealerTable.columns[i].ToJson(col);
			dealer_columns.append(col);
		}
		setting["dealer_columns"] = dealer_columns;
		preference_storage.ToJson(setting["overlays"]["AWIO"]);
	}
	virtual void FromJson(Json::Value& setting) {

		Json::Value dealer_columns = setting.get("dealer_columns", Json::nullValue);
		std::string dealer_columns_str = "dealer_columns";
		if (setting.find(&*dealer_columns_str.begin(), &*dealer_columns_str.begin() + dealer_columns_str.size()) != nullptr)
		{
			dealerTable.columns.resize(3);
			for (auto i = dealer_columns.begin();
				i != dealer_columns.end();
				++i)
			{
				Table::Column col;
				col.FromJson(*i);
				dealerTable.columns.push_back(col);
			}
		}

		preference_storage.FromJson(setting["overlays"]["AWIO"]);
		
		if (root)
		{
			bool migrationed = false;
			for (auto i = color_category_map.begin(); i != color_category_map.end(); ++i)
			{
				//if (i->first != "UI")
				{
					{
						auto k = root->preference_storage.color_map.find(i->first);
						if (k != root->preference_storage.color_map.end())
						{
							preference_storage.color_map[i->first] = k->second;
							root->preference_storage.color_map.erase(k);
						}
					}
					for (auto j = i->second.begin(); j != i->second.end(); ++j)
					{
						auto k = root->preference_storage.color_map.find(*j);
						if (k != root->preference_storage.color_map.end())
						{
							preference_storage.color_map[*j] = k->second;
							root->preference_storage.color_map.erase(k);
							migrationed = true;
						}
					}
				}
			}
		}
	}
	virtual void PreferencesBody()
	{
		if (ImGui::TreeNode("Opacity"))
		{
			static int group_opacity = 255;

			for (auto j = preference_storage.float_map.begin(); j != preference_storage.float_map.end(); ++j)
			{
				if (ImGui::SliderFloat(j->first.c_str(), &j->second, 0.0f, 1.0f))
				{
					Save();
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Colors"))
		{
			if (ImGui::TreeNode("Group"))
			{
				for (auto j = color_category_map.begin(); j != color_category_map.end(); ++j)
				{
					if (ImGui::ColorEdit3(j->first.c_str(), (float*)&preference_storage.color_map[j->first]))
					{
						for (auto k = color_category_map[j->first].begin();
							k != color_category_map[j->first].end();
							++k)
						{
							{
								preference_storage.color_map[*k] = preference_storage.color_map[j->first];
							}
						}
						Save();
					}
				}
				ImGui::TreePop();
			}

			for (auto j = color_category_map.begin(); j != color_category_map.end(); ++j)
			{
				if (ImGui::TreeNode(j->first.c_str()))
				{
					for (auto k = j->second.begin(); k != j->second.end(); ++k)
					{
						std::string name = *k;
						std::string name_lower = boost::to_lower_copy(*k);
						{
							ImVec4& color = preference_storage.color_map[*k];
							if (texture && images)
							{
								auto im = images->find(name_lower);
								if (im != images->end())
								{
									ImGui::Image(texture, ImVec2(20, 20), im->second.uv0, im->second.uv1);
									ImGui::SameLine();
								}
								else
								{
									if ((im = images->find("empty")) != images->end())
									{
										ImGui::Image(texture, ImVec2(20, 20), im->second.uv0, im->second.uv1);
										ImGui::SameLine();
									}
									else
									{
									}
								}
							}
							if (ImGui::ColorEdit3(name.c_str(), (float*)&color))
							{
								Save();
							}
						}
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}
	virtual void SetTexture(ImTextureID texture, const std::unordered_map<std::string, Image>* images)
	{
		this->texture = texture;
		this->images = images;
	}
};