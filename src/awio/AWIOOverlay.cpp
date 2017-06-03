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
	"Acn"
	},
	},
	{"Healer", {
	"Whm",
	"Cnj",
	"Sch",
	"Ast"
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
