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

static int title_background_opacity = 255;
static int background_opacity = 255;
static int text_opacity = 255;
static int graph_opacity = 255;
static int toolbar_opacity = 255;

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
static std::map<std::string, Image> overlay_images;
static std::map<std::string, std::vector<std::string>> color_category_map;

static Json::Value overlay_atlas;

class Table
{
public:
	class Column
	{
	public:
		Column(std::string title_, std::string index_, int size_, int sizeWeight_, ImVec2 align_ = ImVec2(0.5f,0.5f), bool visible_ = true)
			: Title(title_)
			, index(index_)
			, size(size_)
			, sizeWeight(sizeWeight_)
			, align(align_)
			, visible(visible_)
		{

		}
		int size = 0;
		int sizeWeight = 0;
		std::string Title = "";
		std::string index = "";
		bool visible = true;
		ImVec2 align = ImVec2(0.5f, 0.5f);
	};

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
				columnSizeFixed += columns[i].size;
			}
		}
		for (int i = 0; i < column_max; ++i) {
			if (columns[i].sizeWeight != 0)
			{
				columns[i].size = ((width - columnSizeFixed) * columns[i].sizeWeight) / columnSizeWeightSum;
			}
		}
	}

	std::vector<Column> columns;
	std::vector<std::vector<std::string> > values;

	int progressColumn = 2;
	float maxValue = 0;
};

typedef std::map<std::string, ImVec4> ColorMapType;
static ColorMapType colorMap;
std::map<std::string, std::string> iconMap;
std::map<std::string, std::string> nameToJobMap;
static std::string Title;
static std::string zone;
static std::string duration;
static std::string rdps;
static std::string rhps;
static Table dealerTable;
static Table healerTable;

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
					im.uv0 = ImVec2((float)im.x / (float)overlay_texture_width,
						(float)im.y / (float)overlay_texture_width);
					im.uv1 = ImVec2((float)(im.x + im.width) / (float)overlay_texture_width,
						(float)(im.y + im.height) / (float)overlay_texture_width);
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

	color_category_map["Etc"].push_back("Limit Break");
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
	nameToJobMap[u8"auto-tourelle tour"] =
		nameToJobMap[u8"selbstschuss-gyrocopter läufer"] =
		nameToJobMap[u8"オートタレット・ルーク"] =
		nameToJobMap[u8"자동포탑 룩"] =
		nameToJobMap[u8"rook autoturret"] =
		"rook";
	//bishop
	nameToJobMap[u8"auto-tourelle fou"] =
		nameToJobMap[u8"selbstschuss-gyrocopter turm"] =
		nameToJobMap[u8"オートタレット・ビショップ"] =
		nameToJobMap[u8"자동포탑 비숍"] =
		nameToJobMap[u8"bishop autoturret"] =
		"bishop";
	//emerald
	nameToJobMap[u8"emerald carbuncle"] =
		nameToJobMap[u8"카벙클 에메랄드"] =
		nameToJobMap[u8"カーバンクル・エメラルド"] =
		"emerald";
	//topaz
	nameToJobMap[u8"topaz carbuncle"] =
		nameToJobMap[u8"카벙클 토파즈"] =
		nameToJobMap[u8"カーバンクル・トパーズ"] =
		"topaz";
	//eos
	nameToJobMap[u8"eos"] =
		nameToJobMap[u8"フェアリー・エオス"] =
		nameToJobMap[u8"요정 에오스"] =
		"eos";
	//selene
	nameToJobMap[u8"selene"] =
		nameToJobMap[u8"フェアリー・セレネ"] =
		nameToJobMap[u8"요정 셀레네"] =
		"selene";
	//garuda
	nameToJobMap[u8"garuda-egi"] =
		nameToJobMap[u8"ガルーダ・エギ"] =
		nameToJobMap[u8"가루다 에기"] =
		"garuda";
	//ifrit
	nameToJobMap[u8"ifrit-egi"] =
		nameToJobMap[u8"イフリート・エギ"] =
		nameToJobMap[u8"이프리트 에기"] =
		"ifrit";
	//titan
	nameToJobMap[u8"titan-egi"] =
		nameToJobMap[u8"タイタン・エギ"] =
		nameToJobMap[u8"타이탄 에기"] =
		"titan";

	nameToJobMap[u8"Limit Break"] = "lmb";

	// default color map
	colorMap["TitleText"] = htmlCodeToImVec4("ffffff");
	colorMap["GraphText"] = htmlCodeToImVec4("ffffff");
	colorMap["ToolbarBackground"] = htmlCodeToImVec4("999999");
	
	colorMap["Attacker"] = htmlCodeToImVec4("ff0000");
	colorMap["Healer"]   = htmlCodeToImVec4("00ff00");
	colorMap["Tanker"]   = htmlCodeToImVec4("0000ff");

	colorMap["Pld"] = htmlCodeToImVec4("7B9AA2");
	colorMap["Gld"] = htmlCodeToImVec4("7B9AA2");

	colorMap["War"] = htmlCodeToImVec4("A91A16");
	colorMap["Mrd"] = htmlCodeToImVec4("A91A16");

	colorMap["Drk"] = htmlCodeToImVec4("682531");

	colorMap["Mnk"] = htmlCodeToImVec4("B38915");
	colorMap["Pgl"] = htmlCodeToImVec4("B38915");

	colorMap["Drg"] = htmlCodeToImVec4("3752D8");
	colorMap["Lnc"] = htmlCodeToImVec4("3752D8");

	colorMap["Nin"] = htmlCodeToImVec4("EE2E48");
	colorMap["Rog"] = htmlCodeToImVec4("EE2E48");

	colorMap["Brd"] = htmlCodeToImVec4("ADC551");
	colorMap["Arc"] = htmlCodeToImVec4("ADC551");

	colorMap["Mch"] = htmlCodeToImVec4("148AA9");

	colorMap["Blm"] = htmlCodeToImVec4("674598");
	colorMap["Thm"] = htmlCodeToImVec4("674598");


	colorMap["Whm"] = htmlCodeToImVec4("BDBDBD");
	colorMap["Cnj"] = htmlCodeToImVec4("BDBDBD");

	colorMap["Smn"] = htmlCodeToImVec4("32670B");
	colorMap["Acn"] = htmlCodeToImVec4("32670B");
	
	colorMap["Sch"] = htmlCodeToImVec4("32307B");

	colorMap["Ast"] = htmlCodeToImVec4("B1561C");
	colorMap["Limit Break"] = htmlCodeToImVec4("FFBB00");
	colorMap["YOU"] = htmlCodeToImVec4("FF5722");

	colorMap["Background"] = htmlCodeToImVec4("000000");
	colorMap["Background"].w = 0.5;

	colorMap["TitleBackground"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBg];
	colorMap["TitleBackgroundActive"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];
	colorMap["TitleBackgroundCollapsed"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgCollapsed];

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
				background_opacity = setting.get("background_opacity", Json::Int(128)).asInt();
				text_opacity = setting.get("text_opacity", Json::Int(128)).asInt();
				graph_opacity = setting.get("graph_opacity", Json::Int(128)).asInt();
				toolbar_opacity = setting.get("toolbar_opacity", Json::Int(128)).asInt();
				show_name = setting.get("show_name", true).asBool();
				column_max = setting.get("column_max", Json::Int(3)).asInt();
				strcpy(websocket_port, setting.get("websocket_port", "10501").asCString());
				Json::Value color = setting.get("color_map", Json::Value());
				for (auto i = color.begin(); i != color.end(); ++i)
				{
					colorMap[i.key().asString()] = htmlCodeToImVec4(i->asString());
				}

				Json::Value nameToJob = setting.get("name_to_job", Json::nullValue);
				if (nameToJob.size() > 0)
				{
					nameToJobMap.clear();
					for (auto i = nameToJob.begin();
						i != nameToJob.end();
						++i)
					{
						nameToJobMap[i.key().asString()] = i->asString();
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
	setting["background_opacity"] = background_opacity;
	setting["text_opacity"] = text_opacity;
	setting["graph_opacity"] = graph_opacity;
	setting["toolbar_opacity"] = toolbar_opacity;
	setting["show_name"] = show_name;
	setting["column_max"] = column_max;
	setting["websocket_port"] = websocket_port;

	Json::Value nameToJob;
	for (auto i = nameToJobMap.begin();
		i != nameToJobMap.end();
		++i)
	{
		nameToJob[i->first] = i->second;
	}
	setting["name_to_job"] = nameToJob;

	Json::Value color;
	for (auto i = colorMap.begin(); i != colorMap.end(); ++i)
	{
		color[i->first] = ImVec4TohtmlCode(i->second);
	}
	setting["color_map"] = color;

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

void RenderTable(Table& table)
{
	// hard coded....
	const int height = 20;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DC.CursorPos;
	const ImGuiStyle& style = ImGui::GetStyle();
	ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(colorMap["GraphText"], (float)text_opacity / 255.0f));

	int windowWidth = ImGui::GetWindowSize().x - style.ItemInnerSpacing.x * 2.0f;
	dealerTable.UpdateColumnWidth(windowWidth, column_max);
	ImGui::Columns(table.columns.size(), "mixed");
	int offset = 0;
	for (int i = 0; i < column_max; ++i)
	{
		ImGui::SetColumnOffset(i, offset);
		offset += table.columns[i].size;
	}
	for (int i = column_max; i < table.columns.size(); ++i)
	{
		ImGui::SetColumnOffset(i, offset);
	}
	for (int i = 0; i < column_max; ++i)
	{
		if (i == 1)
		{
			if (ImGui::SmallButton("Name")) show_name = !show_name;
		}
		else
		{
			ImGui::Text(table.columns[i].Title.c_str());
		}
		ImGui::NextColumn();
	}
	for (int i = column_max; i < table.columns.size(); ++i)
	{
		ImGui::NextColumn();
	}
	ImGui::Columns(1);
	ImGui::Separator();

	int base = ImGui::GetCursorPosY();
	for (int i = 0; i < table.values.size(); ++i)
	{
		std::string& jobStr =  table.values[i][0];
		const std::string& progressStr =  table.values[i][2];
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
			if (splitVec.size() >= 2)
			{
				boost::trim(splitVec[0]);
				auto i = nameToJobMap.find(splitVec[0]);
				if (i != nameToJobMap.end())
				{
					jobStr = i->second;
				}
			}
		}

		if (!jobStr.empty())
		{
			ImVec4 progressColor = ImVec4(0, 0, 0, 1);
			ColorMapType::iterator ji;
			if ((ji = colorMap.find(jobStr)) != colorMap.end())
			{
				progressColor = ji->second;
			}
			else
			{
				progressColor = colorMap["etc"];
			}
		}

		ImVec4 progressColor = ImVec4(0, 0, 0, 1);
		ColorMapType::iterator ji;
		if ((ji = colorMap.find(jobStr)) != colorMap.end())
		{
			progressColor = ji->second;
		}
		else
		{
			progressColor = colorMap["etc"];
		}
		if ((ji = colorMap.find(nameStr)) != colorMap.end())
		{
			progressColor = ji->second;
		}

		progressColor.w = (float)graph_opacity / 255.0f;
		const ImGuiStyle& style = ImGui::GetStyle();
		ImGui::SetCursorPos(ImVec2(0, base + i * height));
		ImVec2 winpos = ImGui::GetWindowPos();
		ImVec2 pos = ImGui::GetCursorPos();
		pos = window->DC.CursorPos;
		ImRect bb(ImVec2(pos.x, pos.y), 
			ImVec2(pos.x + ImGui::GetWindowSize().x, pos.y + height));
		ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(0.5, 0.5, 0.5, 0.0)), true, style.FrameRounding);

		ImRect bb2(pos, ImVec2(pos.x + ImGui::GetWindowSize().x * progress, pos.y + height));
		ImGui::RenderFrame(bb2.Min, bb2.Max, ImGui::GetColorU32(progressColor), true, style.FrameRounding);
		int basex = 0;
		for (int j = 0; j < column_max; ++j)
		{
			ImGui::SetCursorPos(ImVec2(basex + style.ItemInnerSpacing.x, base + i * height));
			ImVec2 winpos = ImGui::GetWindowPos();
			ImVec2 pos = ImGui::GetCursorPos();
			pos = window->DC.CursorPos;
			ImRect clip, align;

			std::string text = table.values[i][j];
			if (j == 0 && overlay_texture)
			{
				std::string icon = boost::to_lower_copy(text);
				std::map<std::string, Image>::iterator im;
				if ((im = overlay_images.find(icon)) != overlay_images.end())
				{
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					window->DrawList->AddImage(overlay_texture, ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + table.columns[j].size + 1, pos.y + height + 1),
						im->second.uv0, im->second.uv1);// , GetColorU32(tint_col));
				}
			}
			else if (j != 1 || show_name)
			{
				//text = iconMap[text];
				//ImGuiCol_Text
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(ImVec4(0, 0, 0, 1), (float)text_opacity / 255.0f));
				ImGui::RenderTextClipped(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + table.columns[j].size + 1, pos.y + height + 1),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					table.columns[j].align,
					nullptr);
				ImGui::PopStyleColor();
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(colorMap["GraphText"], (float)text_opacity / 255.0f));
				ImGui::RenderTextClipped(pos, ImVec2(pos.x + table.columns[j].size, pos.y + height),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					table.columns[j].align,
					nullptr);
				ImGui::PopStyleColor();
			}

			basex += table.columns[j].size;
		}
		ImGui::NewLine();
		ImGui::SetCursorPos(ImVec2(0, base + (i + 1) * height));
	}

	ImGui::Separator();

	ImGui::PopStyleColor(1);
}

extern "C" int ModRender(ImGuiContext* context)
{
	static bool show_preferences = false;
	try {
		ImGui::SetCurrentContext(context);

		{
			
			int next_column_max = column_max;
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorWithAlpha(colorMap["Background"], (float)background_opacity / 255.0f));
			ImGui::PushStyleColor(ImGuiCol_TitleBg, ColorWithAlpha(colorMap["TitleBackground"], (float)title_background_opacity / 255.0f));
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ColorWithAlpha(colorMap["TitleBackgroundActive"], (float)title_background_opacity / 255.0f));
			ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ColorWithAlpha(colorMap["TitleBackgroundCollapsed"], (float)title_background_opacity / 255.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5, 0.5, 0.5, (float)background_opacity / 255.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3, 0.3, 0.3, (float)background_opacity / 255.0f));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0, 0.0, 0.0, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(colorMap["TitleText"], (float)text_opacity / 255.0f));
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
				ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ColorWithAlpha(colorMap["ToolbarBackground"], (float)toolbar_opacity/255.0f)), true, 0);

				// title
				int windowWidth = ImGui::GetWindowSize().x - style.ItemInnerSpacing.x * 2.0f;

				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(colorMap["TitleText"], (float)text_opacity / 255.0f));
				ImGui::Columns(3, "##TitleBar", false);
				Image& cog = overlay_images["cog"];
				if (ImGui::ImageButton(overlay_texture, ImVec2(65 / 2, 60 / 2), cog.uv0, cog.uv1, -1, ImVec4(0, 0, 0, 0), ColorWithAlpha(colorMap["TitleText"], (float)text_opacity / 255.0f)))
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
			mutex.unlock();


			ImGui::End();
			//ImGui::PopStyleVar();
			ImGui::PopStyleColor(8);


			if (show_preferences)
			{
				ImGui::Begin("Preferences", &show_preferences, ImVec2(300, 500), -1, ImGuiWindowFlags_NoCollapse);
				{
					if (ImGui::SmallButton("Mode")) {
						next_column_max = column_max == 9 ? 3 : 9;
						SaveSettings();
					}
					if (ImGui::InputText("WebSocket Port", websocket_port, 50, ImGuiInputTextFlags_CharsDecimal))
					{
						SaveSettings();
					}
					if (ImGui::TreeNode("Opacity"))
					{
						static int group_opacity = 255;

						if (ImGui::SliderInt("Group Opacity(Bg+Text+Graph)", &group_opacity, 10, 255))
						{
							graph_opacity = text_opacity = background_opacity = group_opacity;
							SaveSettings();
						}
						if (ImGui::SliderInt("Title Background Opacity", &title_background_opacity, 0, 255))
						{
							SaveSettings();
						}
						if (ImGui::SliderInt("Background Opacity", &background_opacity, 0, 255))
						{
							SaveSettings();
						}
						if (ImGui::SliderInt("Text Opacity", &text_opacity, 10, 255))
						{
							SaveSettings();
						}
						if (ImGui::SliderInt("Graph Opacity", &graph_opacity, 0, 255))
						{
							SaveSettings();
						}
						if (ImGui::SliderInt("Toolbar Opacity", &toolbar_opacity, 0, 255))
						{
							SaveSettings();
						}
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Colors"))
					{
						if (ImGui::TreeNode("Group"))
						{
							for (auto j = color_category_map.begin(); j != color_category_map.end(); ++j)
							{
								if (ImGui::ColorEdit3(j->first.c_str(), (float*)&colorMap[j->first]))
								{
									for (auto k = color_category_map[j->first].begin();
										k != color_category_map[j->first].end();
										++k)
									{
										{
											colorMap[*k] = colorMap[j->first];
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
										ImVec4& color = colorMap[*k];
										if (overlay_texture)
										{
											std::map<std::string, Image>::iterator im;
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
