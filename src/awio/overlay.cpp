/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include "version.h"
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
#include <beast/websocket/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>


#include <atlstr.h>
#define STR2UTF8(s) (CW2A(CA2W(s), CP_UTF8))

// This source code seems to require refactoring. :<
// Until then, focus on the features. I am not yet familiar with dear imgui.

static boost::mutex font_mutex;
static boost::mutex mutex;
static bool initialized = false;
std::unordered_map<ImGuiContext*, bool> font_inited;

static bool show_name = true;

static char websocket_message[1024] = { 0, };
static char websocket_host[256] = { 0, };
static char websocket_port[256] = { 0, };
static bool websocket_reconnect = true;
static void* overlay_texture = nullptr;
static unsigned char *overlay_texture_filedata = nullptr;
static int overlay_texture_width = 0, overlay_texture_height = 0, overlay_texture_channels = 0;
static ImFont* largeFont = nullptr;
static ImFont* korFont = nullptr;
static ImFont* japFont = nullptr;
static bool need_font_init = true;
static std::map<std::string, ImVec2> windows_default_sizes;

class Serializable
{
public:
	virtual void ToJson(Json::Value& value) const = 0;
	virtual void FromJson(Json::Value& value) = 0;
};

class Font : public Serializable
{
public:
	Font() {}
	Font(std::string fontname, std::string glyph_range, float font_size)
	{
		this->fontname = fontname;
		this->glyph_range = glyph_range;
		this->font_size = font_size;
	}
	std::string fontname;
	std::string glyph_range;
	float font_size;
	void ToJson(Json::Value& value) const
	{
		value["fontname"] = fontname;
		value["glyph_range"] = glyph_range;
		value["font_size"] = font_size;
	}
	void FromJson(Json::Value& value)
	{
		fontname = value["fontname"].asString();
		glyph_range = value["glyph_range"].asString();
		font_size = value["font_size"].asFloat();
	}
};

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

static std::vector<Font> fonts;
static std::unordered_map<std::string, Image> overlay_images;
static std::map<std::string, std::vector<std::string>> color_category_map;
static Json::Value overlay_atlas;
static std::vector<const char*> glyph_range_key;
static std::map<std::string, bool> boolean_map;
static bool& show_status = boolean_map["ShowStatus"];
static bool& movable = boolean_map["Movable"];
static bool& websocket_ssl = boolean_map["WebSocketSSL"];

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
				columns[i].size = std::max(0,((width - columnSizeFixed) * columns[i].sizeWeight) / columnSizeWeightSum) / scale;
			}
			columns[i].offset = offset + (column_margin) * scale;
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
static float& resizegrip_opacity = opacity_map["ResizeGrip"];
static float& graph_opacity = opacity_map["Graph"];
static std::string default_pet_job;

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

class WebSocket
{
public:
	WebSocket() {}
	~WebSocket()
	{
		loop = false;
		if (websocket_thread)
		{
			websocket_thread->join();
			websocket_thread.reset();
		}
	}

	bool loop = false;;
	std::shared_ptr<std::thread> websocket_thread;

	void Process(const std::string& message_str)
	{
		{
			Json::Value root;
			Json::Reader r;

			if (r.parse(message_str, root))
			{
				std::vector<Table::Row> _rows;
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

								ColorMapType::iterator ji;

								// name and job
								if ((ji = color_map.find(nameStr)) != color_map.end())
								{
									row.color = &ji->second;
								}
								else
								{
									if ((ji = color_map.find(jobStr)) != color_map.end())
									{
										row.color = &ji->second;
									}
									else
									{
										row.color = &color_map["etc"];
									}
								}
								_rows.push_back(row);
							}
						}
					}
					dealerTable.rows = _rows;
					dealerTable.values = _values;
					dealerTable.maxValue = _maxValue;
					mutex.unlock();
				}
			}
		}
	}

	void Run()
	{
		auto func = [this]()
		{
			while (loop)
			{
				boost::asio::io_service ios;
				boost::asio::ip::tcp::resolver r{ ios };
				try {
					websocket_reconnect = false;

					// localhost only !
					boost::asio::ip::tcp::socket sock{ ios };
					std::string host = websocket_host;
					auto endpoint = r.resolve(boost::asio::ip::tcp::resolver::query{ host, websocket_port });
					std::string s = endpoint->service_name();
					uint16_t p = endpoint->endpoint().port();
					boost::asio::connect(sock, endpoint);
					
					std::string host_port = host + ":" + websocket_port;
					if (websocket_ssl)
					{
						// Perform SSL handshaking
						using stream_type = boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>;
						boost::asio::ssl::context ctx{ boost::asio::ssl::context::sslv23 };
						stream_type stream{ sock, ctx };
						stream.set_verify_mode(boost::asio::ssl::verify_none);
						stream.handshake(boost::asio::ssl::stream_base::client );

						// Secure WebSocket connect and send message using Beast
						beast::websocket::stream<stream_type&> ws{ stream };
						ws.handshake(host_port, "/MiniParse");

						if (sock.is_open())
						{
							strcpy_s(websocket_message, 1023, "Connected");
						}
						while (sock.is_open() && loop)
						{
							if (websocket_reconnect)
							{
								sock.close();
								std::cout << "Closed" << "\n";
								break;
							}
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
								Process(message_str);
							}
						}
					}
					else
					{
						beast::websocket::stream<boost::asio::ip::tcp::socket&> ws{ sock };
						
						ws.handshake(host_port, "/MiniParse");
						if (sock.is_open())
						{
							strcpy_s(websocket_message, 1023, "Connected");
						}
						while (sock.is_open() && loop)
						{
							if (websocket_reconnect)
							{
								sock.close();
								std::cout << "Closed" << "\n";
								break;
							}
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
								Process(message_str);
							}
						}
					}

				}
				catch (std::exception& e)
				{
					strcpy_s(websocket_message, 1023, STR2UTF8(e.what()));
					std::cerr << e.what() << std::endl;
				}
				catch (...)
				{
					strcpy_s(websocket_message, 1023, "Unknown exception");
				}
				if (!loop)
					break;
				for (int i = 0; i<50 && loop && !websocket_reconnect; ++i)
					Sleep(100);
				if (!loop)
					break;
			}
		};
		loop = false;
		if (websocket_thread)
			websocket_thread->join();
		loop = true;
		websocket_thread = std::make_shared<std::thread>(func);
	}
};

static WebSocket websocket;

inline static void websocketThread()
{
	// only one.
	if(!websocket.websocket_thread)
		websocket.Run();
}

extern "C" int ModInit(ImGuiContext* context)
{
	boost::unique_lock<boost::mutex> l(font_mutex);
	need_font_init = true;

	try {
		ImGui::SetCurrentContext(context);
		WCHAR result[MAX_PATH] = {};
		GetWindowsDirectoryW(result, MAX_PATH);
		boost::filesystem::path p = result;

		GetModuleFileNameW(NULL, result, MAX_PATH);
		boost::filesystem::path m = result;

		// texture
		boost::filesystem::path texture_path = m.parent_path() / "overlay_atlas.png";
		boost::filesystem::path atlas_json_path = m.parent_path() / "overlay_atlas.json";
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
						im.uv0 = ImVec2(((float)im.x + 0.5f) / (float)overlay_texture_width,
							((float)im.y + 0.5f) / (float)overlay_texture_width);
						im.uv1 = ImVec2((float)(im.x + im.width - 1 + 0.5f) / (float)overlay_texture_width,
							(float)(im.y + im.height - 1 + 0.5f) / (float)overlay_texture_width);
						overlay_images[name] = im;
					}
				}
			}
		}

		dealerTable.columns = {
			// fixed
			Table::Column("", "Job", (overlay_texture != nullptr) ? 30 : 20, 0, ImVec2(0.5f, 0.5f)),
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
		// Color category
		color_category_map["Tank"] = {
			"Pld",
			"Gld",
			"War",
			"Mrd",
			"Drk",
		};
		color_category_map["DPS"] = {
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
		};
		color_category_map["Healer"] = {
			"Whm",
			"Cnj",
			"Sch",
			"Ast"
		};
		color_category_map["Etc"] = {
			"limit break",
			"YOU",
			"etc"
		};
		color_category_map["UI"] = {
			"Background",
			"TitleBackground",
			"TitleBackgroundActive",
			"TitleBackgroundCollapsed",
			"ResizeGrip",
			"ResizeGripActive",
			"ResizeGripHovered",
			"TitleText",
			"GraphText",
			"ToolbarBackground"
		};

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

		color_map["DPS"] = htmlCodeToImVec4("ff0000");
		color_map["Healer"] = htmlCodeToImVec4("00ff00");
		color_map["Tank"] = htmlCodeToImVec4("0000ff");

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

		color_map["ResizeGrip"] = ImGui::GetStyle().Colors[ImGuiCol_ResizeGrip];


		default_pet_job = "chocobo";

		//opacity
		global_opacity = 1.0f;
		title_background_opacity = 1.0f;
		resizegrip_opacity = 1.0f;
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

		// font setting
		fonts = {
			Font("consolab.ttf", "Default", 18.0f),
			Font("Default", "Default", 15.0f), // Default will ignore font size.
			Font("ArialUni.ttf", "Japanese", 15.0f),
			Font("NanumBarunGothic.ttf", "Korean", 15.0f),
			Font("gulim.ttc", "Korean", 15.0f),
		};

		// default port
		strcpy(websocket_port, "10501");
		strcpy(websocket_host, "127.0.0.1");
		websocket_ssl = false;

		movable = true;
		show_status = true;

		// default size
		windows_default_sizes["Preferences"] = ImVec2(300, 500);
		windows_default_sizes["AWIO (ActWebSocket ImGui Overlay)"] = ImVec2(300, 500);
		windows_default_sizes["Status"] = ImVec2(300, 50);

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
					default_pet_job = setting.get("default_pet_job", default_pet_job).asString();
					strcpy(websocket_host, setting.get("websocket_host", "127.0.0.1").asCString());
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
					Json::Value boolean = setting.get("boolean_map", Json::Value());
					for (auto i = boolean.begin(); i != boolean.end(); ++i)
					{
						boolean_map[i.key().asString()] = i->asFloat();
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
					Json::Value fonts = setting.get("fonts", Json::nullValue);
					std::string fonts_str = "fonts";
					if (setting.find(&*fonts_str.begin(), &*fonts_str.begin() + fonts_str.size()) != nullptr)
					{
						if (fonts.size() > 0)
						{
							::fonts.clear();
							for (auto i = fonts.begin();
								i != fonts.end();
								++i)
							{
								Font font;
								font.FromJson(*i);
								::fonts.push_back(font);
							}
						}
					}
					Json::Value windows  = setting.get("windows", Json::nullValue);
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
								windows_default_sizes[name] = size;
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
				fin.close();
			}
			catch (...)
			{
			}
		}
		initialized = true;

		websocketThread();

		{
			// Changing or adding fonts at runtime causes a crash.
			// TODO: is it possible ?...

			ImGuiIO& io = ImGui::GetIO();

			WCHAR result[MAX_PATH] = {};
			GetWindowsDirectoryW(result, MAX_PATH);
			boost::filesystem::path p = result;

			GetModuleFileNameW(NULL, result, MAX_PATH);
			boost::filesystem::path m = result;

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
				m.parent_path(), // dll path
				p / "Fonts", // windows fonts
			};

			ImFontConfig config;
			config.MergeMode = false;
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
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
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
	setting["websocket_host"] = websocket_host;
	setting["websocket_port"] = websocket_port;
	setting["default_pet_job"] = default_pet_job;

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

	Json::Value boolean;
	for (auto i = boolean_map.begin(); i != boolean_map.end(); ++i)
	{
		boolean[i->first] = i->second;
	}
	setting["boolean_map"] = boolean;

	Json::Value dealer_columns;
	for (int i = 3; i < dealerTable.columns.size(); ++i)
	{
		Json::Value col;
		dealerTable.columns[i].ToJson(col);
		dealer_columns.append(col);
	}
	setting["dealer_columns"] = dealer_columns;

	Json::Value fonts;
	for (int i = 0; i < ::fonts.size(); ++i)
	{
		Json::Value font;
		::fonts[i].ToJson(font);
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
	std::ofstream fout((m.parent_path() / "mod.json").wstring());
	fout << w.write(setting);
	fout.close();
}


extern "C" int ModUnInit(ImGuiContext* context)
{
	boost::unique_lock<boost::mutex> l(font_mutex);
	ImGui::SetCurrentContext(context);
	SaveSettings();
	initialized = false;
	return 0;
}

extern "C" void ModTextureData(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
	boost::unique_lock<boost::mutex> l(font_mutex);
	assert(out_pixels != nullptr && out_width != nullptr && out_height != nullptr);
	//*out_pixels = nullptr;
	//*out_width = 0;
	//*out_height = 0;
	//return;
	*out_pixels = overlay_texture_filedata;
	*out_width = overlay_texture_width;
	*out_height = overlay_texture_height;
	if (out_bytes_per_pixel)
		*out_bytes_per_pixel = overlay_texture_channels;
}

extern "C" void ModSetTexture(void* texture)
{
	boost::unique_lock<boost::mutex> l(font_mutex);
	overlay_texture = texture;
	if (texture == nullptr)
	{
		need_font_init = true;
		// TODO: deinit texture;
	}
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

	const ImGuiIO& io = ImGui::GetIO();
	int base = ImGui::GetCursorPosY();
	for (int i = 0; i < table.columns.size(); ++i)
	{
		if (!table.columns[i].visible)
			continue;
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
				ImGui::RenderTextClipped(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + (table.columns[i].size + 1) * io.FontGlobalScale, pos.y + height + 1),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					ImVec2(0.5f, 0.5f),
					//table.columns[i].align,
					nullptr);
				ImGui::PopStyleColor();
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(color_map["GraphText"], text_opacity * global_opacity));
				ImGui::RenderTextClipped(pos, ImVec2(pos.x + (table.columns[i].size) * io.FontGlobalScale, pos.y + height),
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

	const ImGuiIO& io = ImGui::GetIO();
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
		ImVec4 progressColor = table.rows[i].color != nullptr ? *table.rows[i].color : color_map["etc"];

		progressColor.w = graph_opacity * global_opacity;
		const ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 winpos = ImGui::GetWindowPos();
		ImVec2 pos = ImGui::GetCursorPos();
		pos = window->DC.CursorPos;

		Image& center = overlay_images["center"];

		{
			int length = (ImGui::GetWindowSize().x) * progress;
			ImRect bb(ImVec2(pos.x, pos.y),
				ImVec2(pos.x + length, pos.y + height));
			window->DrawList->AddImage(overlay_texture, bb.Min, ImVec2(bb.Max.x, pos.y + height),
				center.uv0, center.uv1, ImGui::GetColorU32(progressColor));
		}
		//ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(0.5, 0.5, 0.5, 0.0)), true, style.FrameRounding);
		//ImRect bb2(pos, ImVec2(pos.x + ImGui::GetWindowSize().x * progress, pos.y + height));
		//ImGui::RenderFrame(bb2.Min, bb2.Max, ImGui::GetColorU32(progressColor), true, style.FrameRounding);
		for (int j = 0; j < table.columns.size() && j < table.values[i].size(); ++j)
		{
			if (!table.columns[j].visible)
				continue;
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
					window->DrawList->AddImage(overlay_texture, ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + (table.columns[j].size + 1)*io.FontGlobalScale, pos.y + height + 1),
						im->second.uv0, im->second.uv1);// , GetColorU32(tint_col));
				}
			}
			else if (j != 1 || show_name)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(ImVec4(0, 0, 0, 1), text_opacity * global_opacity));
				ImGui::RenderTextClipped(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + (table.columns[j].size + 1) * io.FontGlobalScale, pos.y + height + 1),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					table.columns[j].align,
					nullptr);
				ImGui::PopStyleColor();
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(color_map["GraphText"], text_opacity * global_opacity));
				ImGui::RenderTextClipped(pos, ImVec2(pos.x + table.columns[j].size * io.FontGlobalScale, pos.y + height),
					text.c_str(),
					text.c_str() + text.size(),
					nullptr,
					table.columns[j].align,
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

void RenderTable(Table& table)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	const ImGuiIO& io = ImGui::GetIO();
	int windowWidth = ImGui::GetWindowContentRegionWidth();
	int column_max = table.columns.size();
	const int height = 20 * io.FontGlobalScale;
	table.UpdateColumnWidth(windowWidth, height, column_max, io.FontGlobalScale);

	ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(color_map["GraphText"], text_opacity * global_opacity));
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
	ImGui::Begin("Preferences", show_preferences, windows_default_sizes["Preferences"], -1, ImGuiWindowFlags_NoCollapse);
	{
		ImGui::Text("Version : %s", VERSION_LONG_STRING);
		ImGui::Text("Github : https://github.com/ZCube/ACTWebSocket");
		ImGui::Text("Github : https://github.com/ZCube/ACTWebSocketOverlay");
		ImGui::Text("");
		if (ImGui::TreeNode("Windows"))
		{
			if (ImGui::Checkbox("Status", &show_status))
			{
				SaveSettings();
			}
			ImGui::TreePop();
		}
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
						SaveSettings();
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
						SaveSettings();
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
						SaveSettings();
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
			if (ImGui::Checkbox("Use SSL", &boolean_map["WebSocketSSL"]))
			{
				websocket_reconnect = true;
				strcpy_s(websocket_message, 1023, "Connecting...");
				SaveSettings();
			}
			
			if (ImGui::InputText("Host", websocket_host, 50))
			{
				websocket_reconnect = true;
				strcpy_s(websocket_message, 1023, "Connecting...");
				SaveSettings();
			}
			if (ImGui::InputText("Port", websocket_port, 50, ImGuiInputTextFlags_CharsDecimal))
			{
				websocket_reconnect = true;
				strcpy_s(websocket_message, 1023, "Connecting...");
				SaveSettings();
			}
			ImGui::TreePop();
		}
		ImGui::Text("WebSocket Status : %s", websocket_message);
		ImGui::Separator();
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
		if (ImGui::TreeNode("Fonts"))
		{
			ImGui::Text("The font settings are applied at the next start.");
			ImGui::Text("Default font is \'Default\'");
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
				static float font_size = 15.0f;
				static int glyph_range = 0;
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
				}
				if (ImGui::Button("Up"))
				{
					if (index > 0)
					{
						std::swap(fonts[index], fonts[index - 1]);
						SaveSettings();
						decIndex = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Down"))
				{
					if (index + 1 < fonts.size())
					{
						std::swap(fonts[index], fonts[index + 1]);
						SaveSettings();
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
					if (ImGui::InputText("FontName", buf, 99))
					{
						fonts[index].fontname = buf;
						SaveSettings();
					}
					if (ImGui::Combo("GlyphRange", &glyph_range, glyph_range_key.data(), glyph_range_key.size()))
					{
						if (glyph_range >= 0)
						{
							fonts[index].glyph_range = glyph_range_key[glyph_range];
							SaveSettings();
						}
					}
					if (ImGui::SliderFloat("Size", &font_size, 5, 100))
					{
						fonts[index].font_size = font_size;
						SaveSettings();
					}
					ImGui::EndPopup();
				}
				ImGui::SameLine();

				if (ImGui::Button("Remove"))
				{
					if (index > 0)
					{
						fonts.erase(fonts.begin() + index);
						SaveSettings();
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
					ImGui::OpenPopup("Append Column");
				}
				if (ImGui::BeginPopup("Append Column"))
				{
					static char buf[100] = { 0, };
					static int glyph_range = -1;
					static float font_size = 15.0f;
					if (ImGui::InputText("Title", buf, 99))
					{
					}
					if (ImGui::Combo("GlyphRange", &glyph_range, glyph_range_key.data(), glyph_range_key.size()))
					{
					}
					if (ImGui::SliderFloat("Size", &font_size, 5, 100))
					{
						SaveSettings();
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
							font_size = 15.0f;
							SaveSettings();
						}
					}
					ImGui::EndPopup();
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
	bool move_key_pressed = GetAsyncKeyState(VK_SHIFT) && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_MENU);
	bool use_input = movable || show_preferences || move_key_pressed;
	try {
		ImGui::SetCurrentContext(context);

		{
			ImVec2 padding(0, 0);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorWithAlpha(color_map["Background"], background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_TitleBg, ColorWithAlpha(color_map["TitleBackground"], title_background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ColorWithAlpha(color_map["TitleBackgroundActive"], title_background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ColorWithAlpha(color_map["TitleBackgroundCollapsed"], title_background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_ResizeGrip, ColorWithAlpha(color_map["ResizeGrip"], resizegrip_opacity));
			ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, ColorWithAlpha(color_map["ResizeGripActive"], resizegrip_opacity));
			ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, ColorWithAlpha(color_map["ResizeGripHovered"], resizegrip_opacity));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5, 0.5, 0.5, background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3, 0.3, 0.3, background_opacity * global_opacity));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0, 0.0, 0.0, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(color_map["TitleText"], text_opacity * global_opacity));
			ImGui::Begin("AWIO (ActWebSocket ImGui Overlay)", nullptr, windows_default_sizes["AWIO (ActWebSocket ImGui Overlay)"], -1,
				ImGuiWindowFlags_NoTitleBar | (use_input ? NULL : ImGuiWindowFlags_NoInputs));

			mutex.lock();

			auto &io = ImGui::GetIO();
			Image& cog = overlay_images["cog"];
			Image& resize = overlay_images["resize"];
			float icon_color_change = (cosf(GetTickCount() / 500.0f) + 1.0f) / 2.0f;
			ImVec4 color = ColorWithAlpha(color_map["TitleText"], text_opacity * global_opacity);
			color.x *= icon_color_change;
			color.y *= icon_color_change;
			color.z *= icon_color_change;
			auto p = ImGui::GetCursorPos();
			ImGui::BeginChild("Btn",
				//ImVec2(100,100),
				ImVec2((cog.width+resize.width+36) * io.FontGlobalScale / 6, (cog.height + 18) * io.FontGlobalScale /6),
				false,
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			if (ImGui::ImageButton(overlay_texture, ImVec2(resize.width * io.FontGlobalScale / 6, resize.height * io.FontGlobalScale / 6), resize.uv0, resize.uv1, 2, ImVec4(0, 0, 0, 0), color))
			{
				movable = !movable;
			}
			ImGui::EndChild();
			ImGui::SameLine((cog.width + 18) * io.FontGlobalScale / 6);
			ImGui::BeginChild("Btn1",
				//ImVec2(100,100),
				ImVec2((cog.width + resize.width + 36) * io.FontGlobalScale / 6, (cog.height + 18) * io.FontGlobalScale / 6),
				false,
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			if (ImGui::ImageButton(overlay_texture, ImVec2(cog.width * io.FontGlobalScale / 6, cog.height * io.FontGlobalScale / 6), cog.uv0, cog.uv1, 2, ImVec4(0, 0, 0, 0), color))
			{
				show_preferences = !show_preferences;
			}
			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::SetCursorPos(p);

			RenderTable(dealerTable);

			ImGui::Separator();

			ImGui::End();
			ImGui::PopStyleVar(1);


			if (show_status)
			{
				ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorWithAlpha(color_map["ToolbarBackground"], background_opacity * global_opacity));
				ImGui::PushStyleColor(ImGuiCol_Text, ColorWithAlpha(color_map["TitleText"], text_opacity * global_opacity));
				ImGui::Begin("Status", nullptr, windows_default_sizes["Status"], -1,
					ImGuiWindowFlags_NoTitleBar | (use_input ? NULL : ImGuiWindowFlags_NoInputs));
				{
					int height = 40 * io.FontGlobalScale;
					const ImGuiStyle& style = ImGui::GetStyle();
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					{
						std::string duration_short = "- " + duration;

						ImVec2 pos = ImGui::GetCursorPos();
						ImGui::Text("%s - %s", zone.c_str(), duration.c_str(), rdps.c_str(), rhps.c_str());
						ImGui::Text("RDPS : %s RHPS : %s", rdps.c_str(), rhps.c_str());

						ImGui::SetCursorPos(pos);
						ImGui::Text(" ");
						ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - (65 + 5) * io.FontGlobalScale / 2);
					}
				}
				ImGui::End();
				ImGui::PopStyleColor(2);
			}


			//ImGui::Separator();

			//ImGui::PopStyleVar();
			ImGui::PopStyleColor(11);


			if (show_preferences)
			{
				auto &io = ImGui::GetIO();
				Preference(context, &show_preferences);
			}
			else
			{
				auto &io = ImGui::GetIO();
				if (!use_input)
				{
					io.WantCaptureMouse = false;
				}
				io.WantCaptureKeyboard = false;
			}
			mutex.unlock();
		}
	}
	catch (std::exception& e)
	{
		ImGui::End();
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
