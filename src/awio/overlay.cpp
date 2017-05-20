#include <imgui.h>
#include "imgui_internal.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <json/json.h>

#include <stdio.h>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <thread>
#include <unordered_map>

#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <beast/core.hpp>
#include <beast/websocket.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>

static boost::mutex mutex;
static bool show_name = true;
static int column_max = 3;

static char websocket_port[256] = { 0, };
static void* overlay_texture = nullptr;
static unsigned char *overlay_texture_filedata = nullptr;
static int overlay_texture_width = 0, overlay_texture_height = 0, overlay_texture_channels = 0;
static ImFont* largeFont;
static ImFont* korFont = nullptr;
static ImFont* japFont = nullptr;

class Image
{
public:
	int x;
	int y;
	int width;
	int height;
	ImVec2 uv0;
	ImVec2 uv1;
	std::vector<int> widths;
	std::vector<int> heights;
};
static std::unordered_map<std::string, Image> overlay_images;
static std::map<std::string, std::vector<std::string>> color_category_map;

static Json::Value overlay_atlas;

class Table
{
public:
	class Column
	{
	public:
		Column(std::string title_ = "", std::string index_ = "", int size_ = 0, int sizeWeight_ = 0, ImVec2 align_ = ImVec2(0.5f,0.5f), bool visible_ = true)
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

	void UpdateColumnWidth(int width, int column_max)
	{
		int columnSizeWeightSum = 0;
		int columnSizeFixed = 0;
		for (int i = 0; i < column_max; ++i) {
			if (columns[i].sizeWeight != 0)
			{
				columnSizeWeightSum += columns[i].sizeWeight;
			}
			else
			{
				columnSizeFixed += columns[i].size + column_margin*2;
			}
		}
		int offset = 0;
		for (int i = 0; i < column_max; ++i) {
			if (columns[i].sizeWeight != 0)
			{
				columns[i].size = std::max(0,((width - columnSizeFixed) * columns[i].sizeWeight) / columnSizeWeightSum);
			}
			columns[i].offset = offset + column_margin;
			offset += columns[i].size + column_margin;
		}

		for (int i = 2; i < column_max; ++i) {
			if (columns[i - 1].offset + columns[i - 1].size > columns[i].offset)
			{
				columns[i].offset = columns[i - 1].offset + columns[i - 1].size;
			}
		}
	}

	std::vector<int> offsets;
	std::vector<Column> columns;
	std::vector<std::vector<std::string> > values;

	int progressColumn = 2;
	float maxValue = 0;
};

typedef std::map<std::string, ImVec4> ColorMapType;
static ColorMapType color_map;
static std::map<std::string, float> opacity_map;
std::map<std::string, std::string> name_to_job_map;
static std::string Title;
static std::string zone;
static std::string duration;
static std::string rdps;
static std::string rhps;
static Table dealerTable;
static Table healerTable;
static std::vector<const char*> combatant_attribute_names;

static float& global_opacity = opacity_map["Global"];
static float& text_opacity = opacity_map["Text"];
static float& background_opacity = opacity_map["Background"];
static float& title_background_opacity = opacity_map["TitleBackground"];
static float& toolbar_opacity = opacity_map["Toolbar"];
static float& graph_opacity = opacity_map["Graph"];

ImVec4 htmlCodeToImVec4(const std::string hex)
{
	unsigned char dat[256] = { 0, };
	boost::algorithm::unhex(hex, dat);
	return ImVec4((float)dat[0] / 255.0f, (float)dat[1] / 255.0f, (float)dat[2] / 255.0f, 1.0f);
}

const std::string ImVec4TohtmlCode(ImVec4 val)
{
	unsigned char dat[256] = { 0, };
	dat[0] = (unsigned char)std::min(255, std::max(0, (int)(val.x * 255.0f)));
	dat[1] = (unsigned char)std::min(255, std::max(0, (int)(val.y * 255.0f)));
	dat[2] = (unsigned char)std::min(255, std::max(0, (int)(val.z * 255.0f)));
	std::string ret;
	boost::algorithm::hex(dat, dat+3, std::back_inserter(ret));
	return ret;
}

inline static void websocketThread()
{
	auto func = []()
	{
		while (true)
		{
			boost::asio::io_service ios;
			boost::asio::ip::tcp::resolver r{ ios };
			try {
				// localhost only !
				std::string const host = "127.0.0.1";
				boost::asio::ip::tcp::socket sock{ ios };
				auto endpoint = r.resolve(boost::asio::ip::tcp::resolver::query{ host, websocket_port });
				std::string s = endpoint->service_name();
				uint16_t p = endpoint->endpoint().port();
				boost::asio::connect(sock, endpoint);

				beast::websocket::stream<boost::asio::ip::tcp::socket&> ws{ sock };
				std::string host_port = host + ":" + websocket_port;
				ws.handshake(host_port, "/MiniParse");

				while (sock.is_open())
				{

					beast::multi_buffer b;
					beast::websocket::opcode op;
					ws.read(op, b);

					std::string message_str;
					message_str = boost::lexical_cast<std::string>(beast::buffers(b.data()));
					// debug
					//std::cout << beast::buffers(b.data()) << "\n";
					b.consume(b.size());
					if (message_str.size() == 1)
					{
						ws.write(boost::asio::buffer(std::string(".")));
					}
					else
					{
						{
							Json::Value root;
							Json::Reader r;

							if (r.parse(message_str, root))
							{
								std::vector<std::vector<std::string> > _values;
								if (root["type"].asString() == "broadcast" && root["msgtype"].asString() == "CombatData")
								{
									float _maxValue = 0;
									mutex.lock();
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

										std::sort(_values.begin(), _values.end(), [](const std::vector<std::string>& vals0, const std::vector<std::string>& vals1)
										{
											return atof(vals0[2].c_str()) > atof(vals1[2].c_str());
										});
									}
									dealerTable.values = _values;
									dealerTable.maxValue = _maxValue;
									mutex.unlock();
								}
							}
						}
					}
				}
			}
			catch (std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}
			catch (...)
			{

			}
			Sleep(5000);
		}
	};
	std::thread* t(new std::thread(func));
}

extern "C" int ModInit(ImGuiContext* context)
{
	ImGui::SetCurrentContext(context);
	WCHAR result[MAX_PATH] = {};
	GetWindowsDirectoryW(result, MAX_PATH);
	boost::filesystem::path p = result;

	GetModuleFileNameW(NULL, result, MAX_PATH);
	boost::filesystem::path m = result;

	// texture
	boost::filesystem::path texture_path = m.parent_path() / "overlay_atlas.png";
	boost::filesystem::path atlas_json_path = m.parent_path() / "overlay_atlas.json";
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
	if (boost::filesystem::exists(atlas_json_path) && overlay_texture_width > 0 && overlay_texture_height > 0)
	{
		FILE *file;
		bool success = false;

		std::ifstream fin(atlas_json_path.wstring().c_str());
		if (fin.is_open())
		{
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
					im.uv0 = ImVec2(((float)im.x+0.5f) / (float)overlay_texture_width,
						((float)im.y+0.5f) / (float)overlay_texture_width);
					im.uv1 = ImVec2((float)(im.x + im.width-1 + 0.5f) / (float)overlay_texture_width,
						(float)(im.y + im.height-1 + 0.5f) / (float)overlay_texture_width);
					overlay_images[name] = im;
				}
			}
		}
	}

	// font
	ImGuiIO& io = ImGui::GetIO();
	ImGui::GetIO().Fonts->AddFontDefault();
	ImFontConfig config;
	config.MergeMode = false;
	ImFontConfig configMerge;
	configMerge.MergeMode = true;
	struct stat buffer;

	static const ImWchar ranges[] =
	{
		0x25A0, 0x25FF,
		0,
	};

	// font loading order...
	//// latin only [0-9a-zA-Z else]?
	if (boost::filesystem::exists(p / "Fonts" / "ArialUni.ttf"))
		japFont = io.Fonts->AddFontFromFileTTF((p / "Fonts" / "ArialUni.ttf").string().c_str(), 15.0f, &configMerge, io.Fonts->GetGlyphRangesJapanese());
	if (boost::filesystem::exists(m.parent_path() / "NanumBarunGothic.ttf"))
		korFont = io.Fonts->AddFontFromFileTTF((m.parent_path() / "NanumBarunGothic.ttf").string().c_str(), 15.0f, &configMerge, io.Fonts->GetGlyphRangesKorean());
	else if (boost::filesystem::exists(p / "Fonts" / "NanumBarunGothic.ttf"))
		korFont = io.Fonts->AddFontFromFileTTF((p / "Fonts" / "NanumBarunGothic.ttf").string().c_str(), 15.0f, &configMerge, io.Fonts->GetGlyphRangesKorean());
	else if (boost::filesystem::exists(p / "Fonts" / "gulim.ttc"))
		korFont = io.Fonts->AddFontFromFileTTF((p / "Fonts" / "gulim.ttc").string().c_str(), 13.0f, &configMerge, io.Fonts->GetGlyphRangesKorean());

	if (boost::filesystem::exists(p / "Fonts" / "consolab.ttf"))
		largeFont = io.Fonts->AddFontFromFileTTF((p / "Fonts" / "consolab.ttf").string().c_str(), 25.0f, &config, io.Fonts->GetGlyphRangesDefault());

	dealerTable.columns.push_back(Table::Column("", "Job", (overlay_texture != nullptr)? 30: 20, 0, ImVec2(0.5f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("Name", "name", 0, 1, ImVec2(0.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("DPS", "encdps", 50, 0, ImVec2(1.0f, 0.5f)));
	// modify
	dealerTable.columns.push_back(Table::Column("D%%", "damage%", 40, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("Damage", "damage", 50, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("Swing", "swings", 40, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("Miss", "misses", 40, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("D.CRIT", "crithit%", 40, 0, ImVec2(1.0f, 0.5f)));
	dealerTable.columns.push_back(Table::Column("Death", "deaths", 40, 0, ImVec2(1.0f, 0.5f)));

	// Color category
	color_category_map["Tanker"].push_back("Pld");
	color_category_map["Tanker"].push_back("Gld");
	color_category_map["Tanker"].push_back("War");
	color_category_map["Tanker"].push_back("Mrd");
	color_category_map["Tanker"].push_back("Drk");

	color_category_map["Attacker"].push_back("Mnk");
	color_category_map["Attacker"].push_back("Pgl");
	color_category_map["Attacker"].push_back("Drg");
	color_category_map["Attacker"].push_back("Lnc");
	color_category_map["Attacker"].push_back("Nin");
	color_category_map["Attacker"].push_back("Rog");
	color_category_map["Attacker"].push_back("Brd");
	color_category_map["Attacker"].push_back("Arc");
	color_category_map["Attacker"].push_back("Mch");
	color_category_map["Attacker"].push_back("Blm");
	color_category_map["Attacker"].push_back("Thm");
	color_category_map["Attacker"].push_back("Smn");
	color_category_map["Attacker"].push_back("Acn");

	color_category_map["Healer"].push_back("Whm");
	color_category_map["Healer"].push_back("Cnj");
	color_category_map["Healer"].push_back("Sch");
	color_category_map["Healer"].push_back("Ast");

	color_category_map["Etc"].push_back("limit break");
	color_category_map["Etc"].push_back("YOU");
	color_category_map["Etc"].push_back("etc");

	color_category_map["UI"].push_back("Background");
	color_category_map["UI"].push_back("TitleBackground");
	color_category_map["UI"].push_back("TitleBackgroundActive");
	color_category_map["UI"].push_back("TitleBackgroundCollapsed");
	color_category_map["UI"].push_back("TitleText");
	color_category_map["UI"].push_back("GraphText");
	color_category_map["UI"].push_back("ToolbarBackground");

	// name to job map
	
	//rook
	name_to_job_map[u8"auto-tourelle tour"] =
		name_to_job_map[u8"selbstschuss-gyrocopter läufer"] =
		name_to_job_map[u8"オートタレット・ルーク"] =
		name_to_job_map[u8"자동포탑 룩"] =
		name_to_job_map[u8"rook autoturret"] =
		"rook";
	//bishop
	name_to_job_map[u8"auto-tourelle fou"] =
		name_to_job_map[u8"selbstschuss-gyrocopter turm"] =
		name_to_job_map[u8"オートタレット・ビショップ"] =
		name_to_job_map[u8"자동포탑 비숍"] =
		name_to_job_map[u8"bishop autoturret"] =
		"bishop";
	//emerald
	name_to_job_map[u8"emerald carbuncle"] =
		name_to_job_map[u8"카벙클 에메랄드"] =
		name_to_job_map[u8"カーバンクル・エメラルド"] =
		"emerald";
	//topaz
	name_to_job_map[u8"topaz carbuncle"] =
		name_to_job_map[u8"카벙클 토파즈"] =
		name_to_job_map[u8"カーバンクル・トパーズ"] =
		"topaz";
	//eos
	name_to_job_map[u8"eos"] =
		name_to_job_map[u8"フェアリー・エオス"] =
		name_to_job_map[u8"요정 에오스"] =
		"eos";
	//selene
	name_to_job_map[u8"selene"] =
		name_to_job_map[u8"フェアリー・セレネ"] =
		name_to_job_map[u8"요정 셀레네"] =
		"selene";
	//garuda
	name_to_job_map[u8"garuda-egi"] =
		name_to_job_map[u8"ガルーダ・エギ"] =
		name_to_job_map[u8"가루다 에기"] =
		"garuda";
	//ifrit
	name_to_job_map[u8"ifrit-egi"] =
		name_to_job_map[u8"イフリート・エギ"] =
		name_to_job_map[u8"이프리트 에기"] =
		"ifrit";
	//titan
	name_to_job_map[u8"titan-egi"] =
		name_to_job_map[u8"タイタン・エギ"] =
		name_to_job_map[u8"타이탄 에기"] =
		"titan";

	name_to_job_map[u8"Limit Break"] = "limit break";

	// default color map
	color_map["TitleText"] = htmlCodeToImVec4("ffffff");
	color_map["GraphText"] = htmlCodeToImVec4("ffffff");
	color_map["ToolbarBackground"] = htmlCodeToImVec4("999999");
	
	color_map["Attacker"] = htmlCodeToImVec4("ff0000");
	color_map["Healer"]   = htmlCodeToImVec4("00ff00");
	color_map["Tanker"]   = htmlCodeToImVec4("0000ff");

	color_map["Pld"] = htmlCodeToImVec4("7B9AA2");
	color_map["Gld"] = htmlCodeToImVec4("7B9AA2");

	color_map["War"] = htmlCodeToImVec4("A91A16");
	color_map["Mrd"] = htmlCodeToImVec4("A91A16");

	color_map["Drk"] = htmlCodeToImVec4("682531");

	color_map["Mnk"] = htmlCodeToImVec4("B38915");
	color_map["Pgl"] = htmlCodeToImVec4("B38915");

	color_map["Drg"] = htmlCodeToImVec4("3752D8");
	color_map["Lnc"] = htmlCodeToImVec4("3752D8");

	color_map["Nin"] = htmlCodeToImVec4("EE2E48");
	color_map["Rog"] = htmlCodeToImVec4("EE2E48");

	color_map["Brd"] = htmlCodeToImVec4("ADC551");
	color_map["Arc"] = htmlCodeToImVec4("ADC551");

	color_map["Mch"] = htmlCodeToImVec4("148AA9");

	color_map["Blm"] = htmlCodeToImVec4("674598");
	color_map["Thm"] = htmlCodeToImVec4("674598");


	color_map["Whm"] = htmlCodeToImVec4("BDBDBD");
	color_map["Cnj"] = htmlCodeToImVec4("BDBDBD");

	color_map["Smn"] = htmlCodeToImVec4("32670B");
	color_map["Acn"] = htmlCodeToImVec4("32670B");
	
	color_map["Sch"] = htmlCodeToImVec4("32307B");

	color_map["Ast"] = htmlCodeToImVec4("B1561C");
	color_map["limit break"] = htmlCodeToImVec4("FFBB00");
	color_map["YOU"] = htmlCodeToImVec4("FF5722");

	color_map["Background"] = htmlCodeToImVec4("000000");
	color_map["Background"].w = 0.5;

	color_map["TitleBackground"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBg];
	color_map["TitleBackgroundActive"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];
	color_map["TitleBackgroundCollapsed"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgCollapsed];

	//opacity
	global_opacity = 1.0f;
	title_background_opacity = 1.0f;
	background_opacity = 1.0f;
	text_opacity = 1.0f;
	graph_opacity = 1.0f;
	toolbar_opacity = 1.0f;

	//combatant attributes
	combatant_attribute_names = {
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

	// default port
	strcpy(websocket_port, "10501");

	{
		GetModuleFileNameW(NULL, result, MAX_PATH);
		boost::filesystem::path m = result;

		Json::Reader r;
		Json::Value setting;
		try {
			std::ifstream fin((m.parent_path() / "mod.json").wstring());
			if (r.parse(fin, setting))
			{
				show_name = setting.get("show_name", true).asBool();
				column_max = setting.get("column_max", Json::Int(3)).asInt();
				strcpy(websocket_port, setting.get("websocket_port", "10501").asCString());
				Json::Value color = setting.get("color_map", Json::Value());
				for (auto i = color.begin(); i != color.end(); ++i)
				{
					color_map[i.key().asString()] = htmlCodeToImVec4(i->asString());
				}
				Json::Value opacity = setting.get("opacity_map", Json::Value());
				for (auto i = opacity.begin(); i != opacity.end(); ++i)
				{
					opacity_map[i.key().asString()] = i->asFloat();
				}
				Json::Value nameToJob = setting.get("name_to_job", Json::nullValue);
				std::string name_to_job_str = "name_to_job";
				if (nameToJob.find(&*name_to_job_str.begin(), &*name_to_job_str.begin() + name_to_job_str.size()) != nullptr)
				{
					name_to_job_map.clear();
					for (auto i = nameToJob.begin();
						i != nameToJob.end();
						++i)
					{
						name_to_job_map[i.key().asString()] = i->asString();
					}
				}
				Json::Value dealer_columns = setting.get("dealer_columns", Json::nullValue);
				std::string dealer_columns_str = "dealer_columns";
				if (setting.find(&*dealer_columns_str.begin(), &*dealer_columns_str.begin()+ dealer_columns_str.size()) != nullptr)
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
			}
			fin.close();
		}
		catch (...)
		{
		}
	}
	websocketThread();
	return 0;
}

void SaveSettings()
{
	WCHAR result[MAX_PATH] = {};
	GetModuleFileNameW(NULL, result, MAX_PATH);
	boost::filesystem::path m = result;

	Json::StyledWriter w;
	Json::Value setting;
	setting["show_name"] = show_name;
	setting["column_max"] = column_max;
	setting["websocket_port"] = websocket_port;

	Json::Value nameToJob;
	for (auto i = name_to_job_map.begin();
		i != name_to_job_map.end();
		++i)
	{
		nameToJob[i->first] = i->second;
	}
	setting["name_to_job"] = nameToJob;

	Json::Value color;
	for (auto i = color_map.begin(); i != color_map.end(); ++i)
	{
		color[i->first] = ImVec4TohtmlCode(i->second);
	}
	setting["color_map"] = color;

	Json::Value opacity;
	for (auto i = opacity_map.begin(); i != opacity_map.end(); ++i)
	{
		opacity[i->first] = i->second;
	}
	setting["opacity_map"] = opacity;

	Json::Value dealer_columns;
	for (int i = 3; i < dealerTable.columns.size(); ++i)
	{
		Json::Value col;
		dealerTable.columns[i].ToJson(col);
		dealer_columns.append(col);
	}
	setting["dealer_columns"] = dealer_columns;

	std::ofstream fout((m.parent_path() / "mod.json").wstring());
	fout << w.write(setting);
	fout.close();
}

extern "C" int ModUnInit(ImGuiContext* context)
{
	ImGui::SetCurrentContext(context);
	SaveSettings();
	return 0;
}

extern "C" void ModTextureData(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
	assert(out_pixels != nullptr && out_width != nullptr && out_height != nullptr);
	//*out_pixels = nullptr;
	*out_pixels = overlay_texture_filedata;
	*out_width = overlay_texture_width;
	*out_height = overlay_texture_height;
	if (out_bytes_per_pixel)
		*out_bytes_per_pixel = overlay_texture_channels;
}

extern "C" void ModSetTexture(void* texture)
{
	overlay_texture = texture;
}

ImVec4 ColorWithAlpha(ImVec4 col, float alpha)
{
	col.w = alpha;
	return col;
}

void RenderTableColumnHeader(Table& table, int height)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	int base = ImGui::GetCursorPosY();
	for (int i = 0; i < table.columns.size(); ++i)
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 winpos = ImGui::GetWindowPos();
		ImVec2 pos = ImGui::GetCursorPos();
		pos = window->DC.CursorPos;

		{
			ImGui::SetCursorPos(ImVec2(table.columns[i].offset + style.ItemInnerSpacing.x, base));
			ImVec2 winpos = ImGui::GetWindowPos();
			ImVec2 pos = ImGui::GetCursorPos();
			pos = window->DC.CursorPos;
			ImRect clip, align;

			std::string text = table.columns[i].Title;
			if (i == 1)
			{
				if (ImGui::Button("Name")) show_name = !show_name;
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(ImVec4(0, 0, 0, 1), text_opacity * global_opacity));
				ImGui::RenderTextClipped(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + table.columns[i].size + 1, pos.y + height + 1),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					ImVec2(0.5f, 0.5f),
					//table.columns[i].align,
					nullptr);
				ImGui::PopStyleColor();
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(color_map["GraphText"], text_opacity * global_opacity));
				ImGui::RenderTextClipped(pos, ImVec2(pos.x + table.columns[i].size, pos.y + height),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					ImVec2(0.5f, 0.5f),
					//table.columns[i].align,
					nullptr);
				ImGui::PopStyleColor();
			}
		}
		ImGui::NewLine();
	}
	ImGui::SetCursorPos(ImVec2(0, base + height));
}

void RenderTableRow(Table& table, int row, int height)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	int offset = 0;
	int base = ImGui::GetCursorPosY();
	ImGui::SetCursorPos(ImVec2(0, base));

	int i = row;
	{
		std::string& jobStr = table.values[i][0];
		const std::string& progressStr = table.values[i][2];
		std::string& nameStr = table.values[i][1];

		if (table.maxValue == 0.0)
			table.maxValue = 1.0;
		float progress = atof(progressStr.c_str()) / table.maxValue;

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
			}
		}

		if (!jobStr.empty())
		{
			ImVec4 progressColor = ImVec4(0, 0, 0, 1);
			ColorMapType::iterator ji;
			if ((ji = color_map.find(jobStr)) != color_map.end())
			{
				progressColor = ji->second;
			}
			else
			{
				progressColor = color_map["etc"];
			}
		}

		ImVec4 progressColor = ImVec4(0, 0, 0, 1);
		ColorMapType::iterator ji;
		if ((ji = color_map.find(jobStr)) != color_map.end())
		{
			progressColor = ji->second;
		}
		else
		{
			progressColor = color_map["etc"];
		}

		progressColor.w = graph_opacity * global_opacity;
		const ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 winpos = ImGui::GetWindowPos();
		ImVec2 pos = ImGui::GetCursorPos();
		pos = window->DC.CursorPos;
		ImRect bb(ImVec2(pos.x, pos.y),
			ImVec2(pos.x + ImGui::GetWindowSize().x, pos.y + height));

		//ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(0.5, 0.5, 0.5, 0.0)), true, style.FrameRounding);
		ImRect bb2(pos, ImVec2(pos.x + ImGui::GetWindowSize().x * progress, pos.y + height));
		//ImGui::RenderFrame(bb2.Min, bb2.Max, ImGui::GetColorU32(progressColor), true, style.FrameRounding);

		Image& left = overlay_images["left"];
		Image& center = overlay_images["center"];
		Image& right = overlay_images["right"];

		{
			ImVec2 p = pos;
			p.x += table.columns[1].offset;
			ImRect bb(ImVec2(p.x, pos.y), ImVec2(p.x + left.width, pos.y + height));
			int length = (ImGui::GetWindowSize().x - left.width - right.width) * progress;

			ImRect bb2(ImVec2(bb.Min.x + left.width, pos.y), ImVec2(bb.Min.x + left.width + length, pos.y + height));
			ImRect bb3(ImVec2(bb.Min.x + left.width + length, pos.y), ImVec2(bb.Min.x + left.width + length + right.width, pos.y + height));

			window->DrawList->AddImage(overlay_texture, bb.Min, ImVec2(bb.Max.x, pos.y + height),
				left.uv0, left.uv1, ImGui::GetColorU32(progressColor));
			window->DrawList->AddImage(overlay_texture, bb2.Min, ImVec2(bb2.Max.x, pos.y + height),
				center.uv0, center.uv1, ImGui::GetColorU32(progressColor));
			window->DrawList->AddImage(overlay_texture, bb3.Min, ImVec2(bb3.Max.x, pos.y + height),
				right.uv0, right.uv1, ImGui::GetColorU32(progressColor));
		}
		//ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(0.5, 0.5, 0.5, 0.0)), true, style.FrameRounding);
		//ImRect bb2(pos, ImVec2(pos.x + ImGui::GetWindowSize().x * progress, pos.y + height));
		//ImGui::RenderFrame(bb2.Min, bb2.Max, ImGui::GetColorU32(progressColor), true, style.FrameRounding);
		for (int j = 0; j < table.columns.size() && j < table.values[i].size(); ++j)
		{
			ImGui::SetCursorPos(ImVec2(table.columns[j].offset + style.ItemInnerSpacing.x, base));
			ImVec2 winpos = ImGui::GetWindowPos();
			ImVec2 pos = ImGui::GetCursorPos();
			pos = window->DC.CursorPos;
			ImRect clip, align;

			std::string text = table.values[i][j];
			if (j == 0 && overlay_texture)
			{
				std::string icon = boost::to_lower_copy(text);
				std::unordered_map<std::string, Image>::iterator im;
				if ((im = overlay_images.find(icon)) != overlay_images.end())
				{
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					window->DrawList->AddImage(overlay_texture, ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + table.columns[j].size + 1, pos.y + height + 1),
						im->second.uv0, im->second.uv1);// , GetColorU32(tint_col));
				}
			}
			else if (j != 1 || show_name)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(ImVec4(0, 0, 0, 1), text_opacity * global_opacity));
				ImGui::RenderTextClipped(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + table.columns[j].size + 1, pos.y + height + 1),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					table.columns[j].align,
					nullptr);
				ImGui::PopStyleColor();
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(color_map["GraphText"], text_opacity * global_opacity));
				ImGui::RenderTextClipped(pos, ImVec2(pos.x + table.columns[j].size, pos.y + height),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					table.columns[j].align,
					nullptr);
				ImGui::PopStyleColor();
			}
		}
		ImGui::NewLine();
	}
	ImGui::SetCursorPos(ImVec2(0, base + height));
}

void RenderTable(Table& table)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	int windowWidth = ImGui::GetWindowSize().x - style.ItemInnerSpacing.x * 2.0f - 10;
	int column_max = table.columns.size();
	table.UpdateColumnWidth(windowWidth, column_max);

	ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(color_map["GraphText"], text_opacity * global_opacity));
	const int height = 20 * ImGui::GetCurrentWindow()->FontWindowScale;
	RenderTableColumnHeader(table, height);

	ImGui::Separator();

	for (int row = 0; row < table.values.size(); ++row)
	{
		RenderTableRow(table, row, height);
	}
	ImGui::PopStyleColor(1);
}

void Preference(ImGuiContext* context, bool* show_preferences)
{
	ImGui::Begin("Preferences", show_preferences, ImVec2(300, 500), -1, ImGuiWindowFlags_NoCollapse);
	{
		if (ImGui::TreeNode("Table"))
		{
			if (ImGui::TreeNode("Dealer Table"))
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
				if (ImGui::Combo("Dest Column", &index_, columns.data(), columns.size()))
				{
					index = index_ >= 0 ? index_ + 3 : index_;
					strcpy(buf, table.columns[index].Title.c_str());
					width = table.columns[index].size;
					align = table.columns[index].align.x;
				}
				ImGui::SameLine();
				if (ImGui::Button("Up"))
				{
					if (index > 3)
					{
						std::swap(table.columns[index], table.columns[index - 1]);
						for (int j = 0; j < table.values.size(); ++j)
						{
							if (table.values[j].size() > index)
							{
								std::swap(table.values[j][index], table.values[j][index - 1]);
							}
						}
						SaveSettings();
						decIndex = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Down"))
				{
					if (index + 1 < table.columns.size())
					{
						std::swap(table.columns[index], table.columns[index + 1]);
						for (int j = 0; j < table.values.size(); ++j)
						{
							if (table.values[j].size() > index + 1)
							{
								std::swap(table.values[j][index], table.values[j][index + 1]);
							}
						}
						SaveSettings();
						incIndex = true;
					}
				}
				if (ImGui::Button("Edit Column"))
				{
					ImGui::OpenPopup("Edit Column");
				}
				if (ImGui::BeginPopup("Edit Column"))
				{
					if (ImGui::InputText("Title", buf, 99))
					{
						table.columns[index].Title = buf;
						SaveSettings();
					}
					if (ImGui::Combo("Attribute", &current_item, combatant_attribute_names.data(), combatant_attribute_names.size()))
					{
						if (strlen(buf) == 0 && current_item >= 0)
						{
							table.columns[index].index = combatant_attribute_names[current_item];
							SaveSettings();
						}
						if (current_item >= 0)
						{
							for (int j = 0; j < table.values.size(); ++j)
							{
								if (table.values[j].size() > index)
								{
									table.values[j][index].clear();
								}
							}
							SaveSettings();
						}
					}
					if (ImGui::SliderInt("Width", &width, 10, 100))
					{
						table.columns[index].size = width;
						SaveSettings();
					}
					if (ImGui::SliderFloat("Align", &align, 0.0f, 1.0f))
					{
						table.columns[index].align.x = align;
						SaveSettings();
					}
					ImGui::EndPopup();
				}
				ImGui::SameLine();

				if (ImGui::Button("Remove Column"))
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
						SaveSettings();
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
				if (ImGui::Button("Append Column"))
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
						if (strlen(buf) == 0 && current_item >= 0)
						{
							strcpy(buf, combatant_attribute_names[current_item]);
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
							current_item = -1;
							width = 50;
							align = 0.5f;
							SaveSettings();
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
		if (ImGui::TreeNode("WebSocket"))
		{
			if (ImGui::InputText("WebSocket Port", websocket_port, 50, ImGuiInputTextFlags_CharsDecimal))
			{
				SaveSettings();
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Opacity"))
		{
			static int group_opacity = 255;

			for (auto j = opacity_map.begin(); j != opacity_map.end(); ++j)
			{
				if (ImGui::SliderFloat(j->first.c_str(), &j->second, 0.0f, 1.0f))
				{
					SaveSettings();
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
					if (ImGui::ColorEdit3(j->first.c_str(), (float*)&color_map[j->first]))
					{
						for (auto k = color_category_map[j->first].begin();
							k != color_category_map[j->first].end();
							++k)
						{
							{
								color_map[*k] = color_map[j->first];
							}
						}
						SaveSettings();
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
							ImVec4& color = color_map[*k];
							if (overlay_texture)
							{
								std::unordered_map<std::string, Image>::iterator im;
								if ((im = overlay_images.find(name_lower)) != overlay_images.end())
								{
									ImGui::Image(overlay_texture, ImVec2(20, 20), im->second.uv0, im->second.uv1);
									ImGui::SameLine();
								}
								else
								{
									if ((im = overlay_images.find("empty")) != overlay_images.end())
									{
										ImGui::Image(overlay_texture, ImVec2(20, 20), im->second.uv0, im->second.uv1);
										ImGui::SameLine();
									}
									else
									{
									}
								}
							}
							if (ImGui::ColorEdit3(name.c_str(), (float*)&color))
							{
								SaveSettings();
							}
						}
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();
}

extern "C" int ModRender(ImGuiContext* context)
{
	static bool show_preferences = false;
	try {
		ImGui::SetCurrentContext(context);

		{
			
			int next_column_max = column_max;
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorWithAlpha(color_map["Background"], background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_TitleBg, ColorWithAlpha(color_map["TitleBackground"], title_background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ColorWithAlpha(color_map["TitleBackgroundActive"], title_background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ColorWithAlpha(color_map["TitleBackgroundCollapsed"], title_background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5, 0.5, 0.5, background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3, 0.3, 0.3, background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0, 0.0, 0.0, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(color_map["TitleText"], text_opacity * global_opacity));
			//ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
			ImGui::Begin("AWIO (ActWebSocket ImGui Overlay)", nullptr, ImVec2(300, 500), -1,
				//ImGuiWindowFlags_NoTitleBar);
				NULL);

			mutex.lock();

			RenderTable(dealerTable);

			ImGui::Separator();
			{
				const ImGuiStyle& style = ImGui::GetStyle();
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				ImVec2 winsize = ImGui::GetWindowSize();
				//ImGui::SetCursorPos(ImVec2(0, base + i * height));
				ImVec2 winpos = ImGui::GetWindowPos();
				ImVec2 pos = ImGui::GetCursorPos();
				pos = window->DC.CursorPos;
				ImRect bb(ImVec2(pos.x-style.ItemInnerSpacing.x,pos.y), ImVec2(pos.x + winsize.x, pos.y + 40));
				ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ColorWithAlpha(color_map["ToolbarBackground"], toolbar_opacity * global_opacity)), true, 0);

				// title
				int windowWidth = ImGui::GetWindowSize().x - style.ItemInnerSpacing.x * 2.0f;

				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(color_map["TitleText"], text_opacity * global_opacity));
				ImGui::Columns(3, "##TitleBar", false);
				Image& cog = overlay_images["cog"];
				if (ImGui::ImageButton(overlay_texture, ImVec2(65 / 2, 60 / 2), cog.uv0, cog.uv1, -1, ImVec4(0, 0, 0, 0), ColorWithAlpha(color_map["TitleText"], text_opacity * global_opacity)))
				{
					show_preferences = !show_preferences;
				}
				ImGui::PushFont(largeFont);
				ImGui::NextColumn();
				ImGui::SetColumnOffset(1, 50);
				std::string duration_short = duration;
				if (duration_short.size() > 5)
				{
					duration_short = duration.substr(duration_short.size() - 5);
				}
				ImGui::Text(duration_short.c_str());
				ImGui::PopFont();
				ImGui::NextColumn();
				ImGui::SetColumnOffset(2, 150);
				ImGui::Text(zone.c_str());
				//ImGui::Text(("RD : " + rdps).c_str());
				//ImGui::SameLine();
				//ImGui::Text(("RH : " + rhps).c_str());
				ImGui::NextColumn();
				ImGui::Columns(1);
				ImGui::PopStyleColor();
			}


			ImGui::Separator();


			ImGui::End();
			//ImGui::PopStyleVar();
			ImGui::PopStyleColor(8);


			if (show_preferences)
			{
				Preference(context, &show_preferences);
			}
			mutex.unlock();
			column_max = next_column_max;
		}

	}
	catch (std::exception& e)
	{
		ImGui::End();
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
