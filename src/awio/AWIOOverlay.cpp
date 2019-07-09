/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/
#include "AWIOOverlay.h"

// Color category

std::map<std::string, std::vector<std::string>> AWIOOverlay::color_category_map =
{
	{"Tank", {
	"Pld",
	"Gld",
	"War",
	"Mrd",
	"Drk",
	"Gnb",
	},
	},
	{"DPS", {
	"Mnk",
	"Pgl",
	"Drg",
	"Lnc",
	"Nin",
	"Rog",
	"Brd",
	"Arc",
	"Mch",
	"Blm",
	"Thm",
	"Smn",
	"Acn",
	"Rdm",
	"Sam",
	"Dnc",
	},
	},
	{"Healer", {
	"Whm",
	"Cnj",
	"Sch",
	"Ast",
	},
	},
	{"Etc", {
	"limit break",
	"YOU",
	"etc"
	},
	},
	{"UI", {
	"Background",
	"TitleText",
	"GraphText",
	},
	},
};

// name to job map

std::map<std::string, std::string> AWIOOverlay::name_to_job_map =
{
	//rook
	{u8"auto-tourelle tour", u8"rook" },
	{u8"selbstschuss-gyrocopter läufer", u8"rook" },
	{u8"オートタレット・ルーク", u8"rook" },
	{u8"자동포탑 룩", u8"rook" },
	{u8"rook autoturret", u8"rook" },
	//bishop
	{u8"auto-tourelle fou", u8"bishop" },
	{u8"selbstschuss-gyrocopter turm", u8"bishop" },
	{u8"オートタレット・ビショップ", u8"bishop" },
	{u8"자동포탑 비숍", u8"bishop" },
	{u8"bishop autoturret", u8"bishop" },
	//emerald
	{u8"emerald carbuncle", u8"emerald" },
	{u8"카벙클 에메랄드", u8"emerald" },
	{u8"カーバンクル・エメラルド", u8"emerald" },
	//topaz
	{u8"topaz carbuncle", u8"topaz" },
	{u8"카벙클 토파즈", u8"topaz" },
	{u8"カーバンクル・トパーズ", u8"topaz" },
	//eos
	{u8"eos", u8"eos" },
	{u8"フェアリー・エオス", u8"eos" },
	{u8"요정 에오스", u8"eos" },
	//selene
	{u8"selene", u8"selene" },
	{u8"フェアリー・セレネ", u8"selene" },
	{u8"요정 셀레네", u8"selene" },
	//garuda
	{u8"garuda-egi", u8"garuda" },
	{u8"ガルーダ・エギ", u8"garuda" },
	{u8"가루다 에기", u8"garuda" },
	//ifrit
	{u8"ifrit-egi", u8"ifrit" },
	{u8"イフリート・エギ", u8"ifrit" },
	{u8"이프리트 에기", u8"ifrit" },
	//titan
	{u8"titan-egi", u8"titan" },
	{u8"タイタン・エギ", u8"titan" },
	{u8"타이탄 에기", u8"titan" },

	{u8"Limit Break", u8"limit break" },
};
//
//// default color map
//color_map["TitleText"] = htmlCodeToImVec4("ffffff");
//color_map["GraphText"] = htmlCodeToImVec4("ffffff");
//color_map["ToolbarBackground"] = htmlCodeToImVec4("999999");
//
//color_map["DPS"] = htmlCodeToImVec4("ff0000");
//color_map["Healer"] = htmlCodeToImVec4("00ff00");
//color_map["Tank"] = htmlCodeToImVec4("0000ff");
//
//color_map["Pld"] = htmlCodeToImVec4("7B9AA2");
//color_map["Gld"] = htmlCodeToImVec4("7B9AA2");
//
//color_map["War"] = htmlCodeToImVec4("A91A16");
//color_map["Mrd"] = htmlCodeToImVec4("A91A16");
//
//color_map["Drk"] = htmlCodeToImVec4("682531");
//
//color_map["Mnk"] = htmlCodeToImVec4("B38915");
//color_map["Pgl"] = htmlCodeToImVec4("B38915");
//
//color_map["Drg"] = htmlCodeToImVec4("3752D8");
//color_map["Lnc"] = htmlCodeToImVec4("3752D8");
//
//color_map["Nin"] = htmlCodeToImVec4("EE2E48");
//color_map["Rog"] = htmlCodeToImVec4("EE2E48");
//
//color_map["Brd"] = htmlCodeToImVec4("ADC551");
//color_map["Arc"] = htmlCodeToImVec4("ADC551");
//
//color_map["Mch"] = htmlCodeToImVec4("148AA9");
//
//color_map["Blm"] = htmlCodeToImVec4("674598");
//color_map["Thm"] = htmlCodeToImVec4("674598");
//
//
//color_map["Whm"] = htmlCodeToImVec4("BDBDBD");
//color_map["Cnj"] = htmlCodeToImVec4("BDBDBD");
//
//color_map["Smn"] = htmlCodeToImVec4("32670B");
//color_map["Acn"] = htmlCodeToImVec4("32670B");
//
//color_map["Sch"] = htmlCodeToImVec4("32307B");
//
//color_map["Ast"] = htmlCodeToImVec4("B1561C");
//color_map["limit break"] = htmlCodeToImVec4("FFBB00");
//color_map["YOU"] = htmlCodeToImVec4("FF5722");
//
//color_map["Background"] = htmlCodeToImVec4("000000");
//color_map["Background"].w = 0.5;
//
//color_map["TitleBackground"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBg];
//color_map["TitleBackgroundActive"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];
//color_map["TitleBackgroundCollapsed"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgCollapsed];
//
//color_map["ResizeGrip"] = ImGui::GetStyle().Colors[ImGuiCol_ResizeGrip];
//
//
std::string AWIOOverlay::default_pet_job = "chocobo";
//
////opacity
//global_opacity = 1.0f;
//title_background_opacity = 1.0f;
//resizegrip_opacity = 1.0f;
//background_opacity = 1.0f;
//text_opacity = 1.0f;
//graph_opacity = 1.0f;
//toolbar_opacity = 1.0f;

//combatant attributes
std::vector<const char*> AWIOOverlay::combatant_attribute_names = {
	//"icon",
	//"display_name",
	//"owner",
	"absorbHeal",
	"BlockPct",
	"critheal%",
	"critheals",
	"crithit%",
	"crithits",
	"cures",
	"damage",
	"damage%",
	"DAMAGE-k",
	"DAMAGE-m",
	"damageShield",
	"damagetaken",
	"deaths",
	"dps",
	"DPS-k",
	"duration",
	"encdps",
	"ENCDPS-k",
	"enchps",
	"ENCHPS-k",
	"healed",
	"healed%",
	"heals",
	"healstaken",
	"hitfailed",
	"hits",
	"IncToHit",
	"Job",
	"kills",
	"Last10DPS",
	"Last180DPS",
	"Last30DPS",
	"Last60DPS",
	"MAXHEAL",
	"MAXHEALWARD",
	"MAXHIT",
	"misses",
	"overHeal",
	"OverHealPct",
	"ParryPct",
	"powerdrain",
	"powerheal",
	"swings",
	"threatdelta",
	"threatstr",
	"TOHIT" };

void AWIOOverlay::Process(const std::string & message_str)
{
	Json::Value root;
	Json::Reader r;

	if (r.parse(message_str, root))
	{
		std::vector<Table::Row> _rows;
		std::vector<std::vector<std::string> > _values;
		//{"type":"broadcast", "msgtype" : "SendCharName", "from" : null, "to" : "uuid", "msg" : {"charID":00, "charName" : "charname"}}
		if (root["type"].asString() == "broadcast" && root["msgtype"].asString() == "SendCharName")
		{
			your_name = root["msg"]["charName"].asString();
		}

		if (root["type"].asString() == "broadcast" && root["msgtype"].asString() == "CombatData")
		{
			float _maxValue = 0;
			processed_data_mutex.lock();
			Table& table = dealerTable;
			{
				_values.clear();
				Json::Value combatant = root["msg"]["Combatant"];
				Json::Value encounter = root["msg"]["Encounter"];

				rdps = encounter["encdps"].asString();
				rhps = encounter["enchps"].asString();

				zone = encounter["CurrentZoneName"].asString();
				duration = encounter["duration"].asString();

				for (auto i = combatant.begin(); i != combatant.end(); ++i)
				{
					std::vector<std::string> vals;
					for (int j = 0; j < table.columns.size(); ++j)
					{
						vals.push_back((*i)[table.columns[j].index].asString());
					}
					_maxValue = std::max<float>(atof(vals[table.progressColumn].c_str()), _maxValue);

					_values.push_back(vals);
				}

				// sort first
				std::sort(_values.begin(), _values.end(), [](const std::vector<std::string>& vals0, const std::vector<std::string>& vals1)
				{
					return atof(vals0[2].c_str()) > atof(vals1[2].c_str());
				});

				for (auto i = 0; i < _values.size(); ++i)
				{
					auto& vals = _values[i];
					Table::Row row;
					{
						std::string& jobStr = vals[0];
						const std::string& progressStr = vals[2];
						std::string& nameStr = vals[1];
						if (jobStr.empty())
						{
							std::string jobStrAlter;
							typedef std::vector< std::string > split_vector_type;
							split_vector_type splitVec; // #2: Search for tokens
							boost::split(splitVec, nameStr, boost::is_any_of("()"), boost::token_compress_on);
							if (splitVec.size() >= 1)
							{
								boost::trim(splitVec[0]);
								auto i = name_to_job_map.find(splitVec[0]);
								if (i != name_to_job_map.end())
								{
									jobStr = i->second;
								}
								else if (splitVec.size() > 1)
								{
									jobStr = default_pet_job;
								}
							}
						}

						auto ji = preference_storage.color_map.find(nameStr);

						// name and job
						if (ji != preference_storage.color_map.end())
						{
							row.color = &ji->second;
						}
						else
						{
							if ((ji = preference_storage.color_map.find(jobStr)) != preference_storage.color_map.end())
							{
								row.color = &ji->second;
							}
							else
							{
								row.color = &preference_storage.color_map["etc"];
							}
						}
						if (nameStr == "YOU" && !your_name.empty())
						{
							nameStr = your_name;
						}
						_rows.push_back(row);
					}
				}
			}
			dealerTable.rows = _rows;
			dealerTable.values = _values;
			dealerTable.maxValue = _maxValue;
			processed_data_mutex.unlock();
		}
	}
}

AWIOOverlay::AWIOOverlay()
{
	websocket_ssl = false;
	websocket_host = "localhost";
	websocket_port = 10501;
	websocket_path = "/MiniParse";
	name = "AWIO";

	dealerTable.columns = {
		// fixed
		Table::Column("", "Job", (texture != nullptr) ? 30 : 20, 0, ImVec2(0.5f, 0.5f)),
		Table::Column("Name", "name", 0, 1, ImVec2(0.0f, 0.5f)),
		Table::Column("DPS", "encdps", 50, 0, ImVec2(1.0f, 0.5f), false),
		// modify
		Table::Column("DPS", "encdps", 50, 0, ImVec2(1.0f, 0.5f)),
		Table::Column("D%%", "damage%", 40, 0, ImVec2(1.0f, 0.5f)),
		Table::Column("Damage", "damage", 50, 0, ImVec2(1.0f, 0.5f)),
		Table::Column("Swing", "swings", 40, 0, ImVec2(1.0f, 0.5f)),
		Table::Column("Miss", "misses", 40, 0, ImVec2(1.0f, 0.5f)),
		Table::Column("D.CRIT", "crithit%", 40, 0, ImVec2(1.0f, 0.5f)),
		Table::Column("Death", "deaths", 40, 0, ImVec2(1.0f, 0.5f)),
	};
}

AWIOOverlay::~AWIOOverlay()
{
	WebSocketStop();
}

bool AWIOOverlay::Init(const boost::filesystem::path & path_) {
	boost::filesystem::path path = boost::filesystem::absolute(path_);
	root_path = path;
	// dps 255 40 51
	// etc 0 0 0
	// healer 53 210 88
	// tank 47 96 255
	// ui 255 255 255
	std::vector<std::pair<std::string, std::string> > default_colors = {
		{ "DPS", "FF2833" },
		{ "Etc", "FFD500" },
		{ "Healer", "35D258" },
		{ "Tank", "2F60FF" },
		{ "UI", "FFFFFF" }
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
}

void AWIOOverlay::Render(bool use_input, OverlayOption * options)
{
	try {
		ImVec2 padding(0, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorWithAlpha(preference_storage.color_map["Background"], background_opacity * global_opacity));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5, 0.5, 0.5, background_opacity * global_opacity));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3, 0.3, 0.3, background_opacity * global_opacity));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0, 0.0, 0.0, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(preference_storage.color_map["TitleText"], text_opacity * global_opacity));
		ImGui::Begin("AWIO", nullptr, options->GetDefaultSize("AWIO", ImVec2(500, 200)), -1,
			ImGuiWindowFlags_NoTitleBar | (use_input ? NULL : ImGuiWindowFlags_NoInputs));
		if (options)
			options->SaveWindowPos();

		processed_data_mutex.lock();

		auto &io = ImGui::GetIO();
		if (images)
		{
			auto cog_img = images->find("cog");
			auto resize_img = images->find("resize");
			if (cog_img != images->end() && resize_img != images->end())
			{
				double scale = (io.Fonts->Fonts[0]->FontSize* io.FontGlobalScale) / cog_img->second.height / 1.5f;
				ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0, 0, 0, 0));
				float icon_color_change = (cosf(GetTickCount() / 500.0f) + 1.0f) / 2.0f;
				ImVec4 color = ColorWithAlpha(preference_storage.color_map["TitleText"], text_opacity * global_opacity);
				color.x *= icon_color_change;
				color.y *= icon_color_change;
				color.z *= icon_color_change;
				auto p = ImGui::GetCursorPos();
				ImGui::BeginChild("Btn",
					//ImVec2(100,100),
					ImVec2((cog_img->second.width + 18) * scale, (cog_img->second.height + 18) * scale),
					false,
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				if (ImGui::ImageButton(texture, ImVec2(resize_img->second.width * scale, resize_img->second.height * scale), resize_img->second.uv0, resize_img->second.uv1, 2, ImVec4(0, 0, 0, 0), color))
				{
					options->movable = !options->movable;
				}
				ImGui::EndChild();
				ImGui::SameLine((cog_img->second.width + 18) * scale);
				ImGui::BeginChild("Btn1",
					//ImVec2(100,100),
					ImVec2((resize_img->second.width + 36) * scale, (cog_img->second.height + 18) * scale),
					false,
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				if (ImGui::ImageButton(texture, ImVec2(cog_img->second.width * scale, cog_img->second.height * scale), cog_img->second.uv0, cog_img->second.uv1, 2, ImVec4(0, 0, 0, 0), color))
				{
					options->show_preferences = !options->show_preferences;
				}
				ImGui::EndChild();
				ImGui::SameLine();
				ImGui::SetCursorPos(p);
				ImGui::PopStyleColor(1);
			}
			else
			{
				double height = (io.Fonts->Fonts[0]->FontSize* io.FontGlobalScale);
				ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0, 0, 0, 0));
				float icon_color_change = (cosf(GetTickCount() / 500.0f) + 1.0f) / 2.0f;
				ImVec4 color = ColorWithAlpha(preference_storage.color_map["TitleText"], text_opacity * global_opacity);
				color.x *= icon_color_change;
				color.y *= icon_color_change;
				color.z *= icon_color_change;
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				auto p = ImGui::GetCursorPos();
				ImGui::BeginChild("Btn",
					//ImVec2(100,100),
					ImVec2(height, height),
					false,
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				if (ImGui::Button("M", ImVec2(height,height)))
				{
					options->movable = !options->movable;
				}
				ImGui::EndChild();
				ImGui::SameLine(height);
				ImGui::BeginChild("Btn1",
					//ImVec2(100,100),
					ImVec2(height, height),
					false,
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				if (ImGui::Button("S", ImVec2(height, height)))
				{
					options->show_preferences = !options->show_preferences;
				}
				ImGui::EndChild();
				ImGui::SameLine();
				ImGui::SetCursorPos(p);
				ImGui::PopStyleColor(2);
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
}

bool AWIOOverlay::IsLoaded()
{
	return true;
}

void AWIOOverlay::Save()
{
	//
	// prepare to save
	//
	PreferenceBase::Save();
}

void AWIOOverlay::Load()
{
	PreferenceBase::Load();
	//
	// load 
	//
}

void AWIOOverlay::ToJson(Json::Value & setting) const {
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

void AWIOOverlay::FromJson(Json::Value & setting) {

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

void AWIOOverlay::PreferencesBody()
{
	if (ImGui::TreeNode("Table"))
	{
		if (ImGui::TreeNode("DPS Table"))
		{
			std::vector<const char*> columns;
			Table& table = dealerTable;
			columns.reserve(table.columns.size() - 3);
			for (int i = 3; i < table.columns.size(); ++i)
			{
				columns.push_back(table.columns[i].Title.c_str());
			}

			static int index_ = -1;
			static char buf[100] = { 0, };
			static int current_item = -1;
			static int width = 50.5f;
			static float align = 0.5f;
			static int index = -1;
			bool decIndex = false;
			bool incIndex = false;
			if (ImGui::ListBox("Column", &index_, columns.data(), columns.size()))
			{
				index = index_ >= 0 ? index_ + 3 : index_;
				strcpy(buf, table.columns[index].Title.c_str());
				width = table.columns[index].size;
				align = table.columns[index].align.x;
				auto ri = std::find(combatant_attribute_names.begin(), combatant_attribute_names.end(), table.columns[index].index);
				current_item = (ri != combatant_attribute_names.end()) ? ri - combatant_attribute_names.begin() : -1;
			}
			if (ImGui::Button("Up"))
			{
				if (index > 3 && index != NULL && index >= 0)
				{
					std::swap(table.columns[index], table.columns[index - 1]);
					for (int j = 0; j < table.values.size(); ++j)
					{
						if (table.values[j].size() > index)
						{
							std::swap(table.values[j][index], table.values[j][index - 1]);
						}
					}
					Save();
					decIndex = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Down"))
			{
				if (index + 1 < table.columns.size() && index != NULL && index >= 0)
				{
					std::swap(table.columns[index], table.columns[index + 1]);
					for (int j = 0; j < table.values.size(); ++j)
					{
						if (table.values[j].size() > index + 1)
						{
							std::swap(table.values[j][index], table.values[j][index + 1]);
						}
					}
					Save();
					incIndex = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Edit"))
			{
				ImGui::OpenPopup("Edit Column");
			}
			if (ImGui::BeginPopup("Edit Column"))
			{
				if (ImGui::InputText("Title", buf, 99))
				{
					table.columns[index].Title = buf;
					Save();
				}
				if (ImGui::Combo("Attribute", &current_item, combatant_attribute_names.data(), combatant_attribute_names.size()))
				{
					if (current_item >= 0)
					{
						if (strlen(buf) == 0)
						{
							strcpy(buf, combatant_attribute_names[current_item]);
						}
						table.columns[index].index = combatant_attribute_names[current_item];
						for (int j = 0; j < table.values.size(); ++j)
						{
							if (table.values[j].size() > index)
							{
								table.values[j][index].clear();
							}
						}
						Save();
					}
				}
				if (ImGui::SliderInt("Width", &width, 10, 100))
				{
					table.columns[index].size = width;
					Save();
				}
				if (ImGui::SliderFloat("Align", &align, 0.0f, 1.0f))
				{
					table.columns[index].align.x = align;
					Save();
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();

			if (ImGui::Button("Remove"))
			{
				if (index > 0)
				{
					table.columns.erase(table.columns.begin() + index);
					for (int j = 0; j < table.values.size(); ++j)
					{
						if (table.values[j].size() > index)
						{
							table.values[j].erase(table.values[j].begin() + index);
						}
					}
					Save();
				}
				if (index >= table.columns.size())
				{
					decIndex = true;
				}
			}
			if (decIndex)
			{
				--index_;
				index = index_ >= 0 ? index_ + 3 : index_;
				if (index >= 0)
				{
					strcpy(buf, table.columns[index].Title.c_str());
					width = table.columns[index].size;
					align = table.columns[index].align.x;
				}
				else
				{
					strcpy(buf, "");
					width = 50;
					align = 0.5f;
				}
			}
			if (incIndex)
			{
				++index_;
				index = index_ >= 0 ? index_ + 3 : index_;
				if (index < table.columns.size())
				{
					strcpy(buf, table.columns[index].Title.c_str());
					width = table.columns[index].size;
					align = table.columns[index].align.x;
				}
				else
				{
					strcpy(buf, "");
					width = 50;
					align = 0.5f;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Append"))
			{
				ImGui::OpenPopup("Append Column");
			}
			if (ImGui::BeginPopup("Append Column"))
			{
				static char buf[100] = { 0, };
				static int current_item = -1;
				static int width = 50.5f;
				static float align = 0.5f;
				if (ImGui::InputText("Title", buf, 99))
				{

				}
				if (ImGui::Combo("Attribute", &current_item, combatant_attribute_names.data(), combatant_attribute_names.size()))
				{
					if (current_item >= 0)
					{
						if (strlen(buf) == 0)
						{
							strcpy(buf, combatant_attribute_names[current_item]);
						}
					}
				}
				if (ImGui::SliderInt("Width", &width, 10, 100))
				{

				}
				if (ImGui::SliderFloat("Align", &align, 0.0f, 1.0f))
				{

				}
				if (ImGui::Button("Append"))
				{
					if (current_item >= 0)
					{
						Table::Column col(buf, combatant_attribute_names[current_item],
							width, 0.0f, ImVec2(align, 0.5f), true);
						table.columns.push_back(col);

						ImGui::CloseCurrentPopup();
						strcpy(buf, "");
						current_item = -1;
						width = 50;
						align = 0.5f;
						Save();
					}
				}
				ImGui::EndPopup();
			}
			ImGui::TreePop();
		}
		//ImGui::Edit
		//dealerTable
		ImGui::TreePop();
	}
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

void AWIOOverlay::SetTexture(ImTextureID texture, const std::unordered_map<std::string, Image>* images)
{
	this->texture = texture;
	this->images = images;
}
