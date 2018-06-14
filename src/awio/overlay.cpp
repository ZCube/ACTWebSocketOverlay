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
#include <boost/beast/core.hpp>
#include "websocket.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/lexical_cast.hpp>
#include "Serializable.h"

#include <atlstr.h>
#define STR2UTF8(s) (CW2A(CA2W(s), CP_UTF8))
#include "../mod.h"
#include "Table.h"

const char* websocket_path = "/MiniParse";
typedef std::map<std::string, ImVec4> ColorMapType;

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
	char websocket_message[1024] = { 0, };
	char websocket_host[256] = { 0, };
	char websocket_port[256] = { 0, };
	bool websocket_reconnect = true;
	bool *websocket_ssl;
public:
	WebSocket(bool* _websocket_ssl) {
		websocket_ssl = _websocket_ssl;
	}
	~WebSocket()
	{
		loop = false;
		if (websocket_thread)
		{
			websocket_thread->join();
			websocket_thread.reset();
		}
	}

	bool loop = false;
	std::shared_ptr<websocket_context_base> websocket;
	std::shared_ptr<std::thread> websocket_thread;

	virtual void Process(const std::string& message_str) = 0;

	void Run()
	{
		auto func = [this]()
		{
			while (loop)
			{
				try {
					websocket_reconnect = false;

					char*buf = websocket_port;
					//char buf[32] = { 0, };
					//sprintf(buf, "%d", websocket_port);

					std::string host = websocket_host;
					boost::asio::io_service ios;
					boost::asio::ip::tcp::resolver r{ ios };
					auto endpoint = r.resolve(boost::asio::ip::tcp::resolver::query{ host, buf });
					std::string s = endpoint->service_name();
					uint16_t p = endpoint->endpoint().port();


#if defined(USE_SSL)
					if (websocket_ssl)
					{
						websocket = std::make_shared<websocket_ssl_context>();
					}
					else
#endif
					{
						websocket = std::make_shared<websocket_context>();
					}

					std::string host_port = host + ":" + buf;

					boost::asio::connect(websocket->sock, endpoint);
					websocket->handshake(host_port, websocket_path);
					{
						if (websocket->sock.is_open())
						{
							strcpy_s(websocket_message, 1023, "Connected");
						}
						boost::beast::multi_buffer b;
						boost::beast::websocket::detail::opcode op;
						//std::function<void(std::string write) > write;
						std::function<void(boost::beast::error_code, std::size_t) > read_handler =
							[this, &read_handler, &b, &op](boost::beast::error_code ec, std::size_t bytes_transferred) {
							if (ec || websocket_reconnect)
							{
								strcpy_s(websocket_message, 1023, "Disconnected");
								//std::cerr << ec.message() << std::endl;
							}
							else
							{
								std::string message_str;
								message_str = boost::lexical_cast<std::string>(boost::beast::buffers(b.data()));
								// debug
								//std::cout << boost::beast::buffers(b.data()) << "\n";
								b.consume(b.size());
								if (message_str.size() == 1)
								{
									websocket->write(boost::asio::buffer(std::string(".")));
								}
								else
								{
									Process(message_str);
								}
								if (loop)
									websocket->async_read(b, websocket->strand.wrap(read_handler));
							}
						};
						if (loop)
							websocket->async_read(b, websocket->strand.wrap(read_handler));
						websocket->ios.run();
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
				for (int i = 0; i < 50 && loop && !websocket_reconnect; ++i)
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

	void Stop() {
		loop = false;
		websocket_reconnect = true;
		if (websocket)
			websocket->close(boost::beast::websocket::none);
		if (websocket_thread)
			websocket_thread->join();
	}
};


class OverlayInstance : public ModInterface, public WebSocket
{
public:
	bool show_preferences = false;
	boost::mutex font_mutex;
	boost::mutex mutex;
	bool initialized = false;
	bool font_setting_dirty = false;
	std::unordered_map<ImGuiContext*, bool> font_inited;

	bool show_name = true;
/*
	char websocket_message[1024];
	char websocket_host[256];
	char websocket_port[256];
	bool websocket_reconnect;
*/

	void* overlay_texture = nullptr;
	unsigned char *overlay_texture_filedata = nullptr;
	int overlay_texture_width = 0, overlay_texture_height = 0, overlay_texture_channels = 0;
	ImFont* largeFont = nullptr;
	ImFont* korFont = nullptr;
	ImFont* japFont = nullptr;
	bool need_font_init = true;
	std::map<std::string, ImVec2> windows_default_sizes;
	float font_sizes = 12.0f;
	std::vector<boost::filesystem::path> font_paths;
	std::vector<std::string> font_filenames;
	std::vector<const char *> font_cstr_filenames;
	std::string your_name = "YOU";

	std::vector<Font> fonts;
	std::unordered_map<std::string, Image> overlay_images;
	std::map<std::string, std::vector<std::string>> color_category_map;
	Json::Value overlay_atlas;
	std::vector<const char*> glyph_range_key;
	std::map<std::string, bool> boolean_map;
	bool& show_status = boolean_map["ShowStatus"];
	bool& movable = boolean_map["Movable"];
	bool& websocket_ssl = boolean_map["WebSocketSSL"];


	ColorMapType color_map;
	std::map<std::string, float> opacity_map;
	std::map<std::string, std::string> name_to_job_map;
	std::string Title;
	std::string zone;
	std::string duration;
	std::string rdps;
	std::string rhps;

	std::vector<const char*> combatant_attribute_names;

	float& global_opacity = opacity_map["Global"];
	float& text_opacity = opacity_map["Text"];
	float& background_opacity = opacity_map["Background"];
	float& title_background_opacity = opacity_map["TitleBackground"];
	float& toolbar_opacity = opacity_map["Toolbar"];
	float& resizegrip_opacity = opacity_map["ResizeGrip"];
	float& graph_opacity = opacity_map["Graph"];
	std::string graph = "center";
	Table dealerTable;

	std::string default_pet_job;

public:
	OverlayInstance() :
		dealerTable(
			global_opacity,
			text_opacity,
			graph_opacity,
			color_map["GraphText"],
			color_map["etc"],
			graph
		),
		WebSocket(&websocket_ssl)
	{

	}
	virtual ~OverlayInstance() {
		boost::unique_lock<boost::mutex> l(mutex);
		Stop();
	}
	virtual int Init(ImGuiContext* context);
	virtual int UnInit(ImGuiContext* context);
	virtual int Render(ImGuiContext* context);
	virtual int TextureBegin();
	virtual void TextureEnd();
	virtual void TextureData(int index, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel);
	virtual bool GetTextureDirtyRect(int index, int dindex, RECT* rect);
	virtual void SetTexture(int index, void* texture);
	virtual bool UpdateFont(ImGuiContext* context);
	virtual bool Menu(bool* show);
	virtual void OverlayInstance::Process(const std::string& message_str);

	void LoadSettings(ImGuiContext* context);
	void SaveSettings();
	void FontsPreferences();
	void Preference(ImGuiContext* context, bool* show_preferences);
	bool LoadFonts(ImGuiContext* context);
	void websocketThread();
};

extern "C" ModInterface* ModCreate()
{
	return new OverlayInstance();
}

extern "C" int ModFree(ModInterface* context)
{
	if (context)
		delete context;
	return 0;
}

int OverlayInstance::Init(ImGuiContext* context)
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

					for (UINT i = 0; i < overlay_texture_width*overlay_texture_height; i += 4)
						std::swap(overlay_texture_filedata[i], overlay_texture_filedata[i + 2]);
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
			"Acn",
			"Rdm",
			"Sam",
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

		color_map["Rdm"] = htmlCodeToImVec4("AC2997");
		color_map["Sam"] = htmlCodeToImVec4("E45A0F");

		color_map["Sch"] = htmlCodeToImVec4("32307B");

		color_map["Ast"] = htmlCodeToImVec4("B1561C");
		color_map["limit break"] = htmlCodeToImVec4("FFBB00");
		color_map["YOU"] = htmlCodeToImVec4("FF5722");

		color_map["Background"] = htmlCodeToImVec4("000000");
		color_map["Background"].w = 0.5;

		ImGuiStyle style;
		ImGui::StyleColorsDark(&style);
		color_map["TitleBackground"] = style.Colors[ImGuiCol_TitleBg];
		color_map["TitleBackgroundActive"] = style.Colors[ImGuiCol_TitleBgActive];
		color_map["TitleBackgroundCollapsed"] = style.Colors[ImGuiCol_TitleBgCollapsed];

		color_map["ResizeGrip"] = style.Colors[ImGuiCol_ResizeGrip];

/*
		color_map["TitleBackground"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBg];
		color_map["TitleBackgroundActive"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];
		color_map["TitleBackgroundCollapsed"] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgCollapsed];

		color_map["ResizeGrip"] = ImGui::GetStyle().Colors[ImGuiCol_ResizeGrip];
*/

		default_pet_job = "chocobo";

		//opacity
		global_opacity = 1.0f;
		title_background_opacity = 0.2f;
		resizegrip_opacity = 1.0f;
		background_opacity = 0.2f;
		text_opacity = 1.0f;
		graph_opacity = 1.0f;
		toolbar_opacity = 0.2f;

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
		strcpy(websocket_host, "localhost");
		websocket_ssl = false;

		movable = true;
		show_status = true;

		LoadSettings(context);
		initialized = true;

		if (windows_default_sizes.empty())
		{
			// default size
			windows_default_sizes["Preferences"] = ImVec2(300, 500);
			windows_default_sizes["AWIO (ActWebSocket ImGui Overlay)"] = ImVec2(300, 500);
			windows_default_sizes["Status"] = ImVec2(300, 50);
		}

		websocketThread();

		LoadFonts(context);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	return 0;
}

void OverlayInstance::LoadSettings(ImGuiContext* context)
{
	{
		WCHAR result[MAX_PATH] = {};
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
				strcpy(websocket_host, setting.get("websocket_host", "localhost").asCString());
				if (strcmp(websocket_host, "127.0.0.1") == 0)
				{
					// for ipv6
					strcpy(websocket_host, "localhost");
				}
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
				// for lua version compat
				Json::Value color2 = setting["overlays"]["AWIO"].get("color_map", Json::Value());
				for (auto i = color2.begin(); i != color2.end(); ++i)
				{
					color_map[i.key().asString()] = htmlCodeToImVec4(i->asString());
				}
				// for lua version compat
				Json::Value opacity2 = setting["overlays"]["AWIO"].get("float_map", Json::Value());
				for (auto i = opacity2.begin(); i != opacity2.end(); ++i)
				{
					opacity2[i.key().asString()] = i->asFloat();
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
							windows_default_sizes[name] = size;
							ImGuiWindowSettings* settings = nullptr;
							if (context)
							{
								ImGuiContext & g = *context;
								g.Initialized = true;
								size = ImMax(size, g.Style.WindowMinSize);
								size = ImMax(size, ImVec2(100,50));
								ImGuiID id = ImHash(name.c_str(), 0);
								{
									for (int i = 0; i != g.SettingsWindows.size(); i++)
									{
										ImGuiWindowSettings* ini = &g.SettingsWindows[i];
										if (ini->Id == id)
										{
											settings = ini;
											break;
										}
									}
									if (settings == nullptr)
									{
										g.SettingsWindows.push_back(ImGuiWindowSettings());
										ImGuiWindowSettings* ini = &g.SettingsWindows.back();
										ini->Name = ImStrdup(name.c_str());
										ini->Id = ImHash(name.c_str(), 0);
										ini->Collapsed = false;
										ini->Pos = ImVec2(FLT_MAX, FLT_MAX);
										ini->Size = ImVec2(0, 0);
										settings = ini;
									}
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
}

void OverlayInstance::SaveSettings()
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
	std::ofstream fout((m.parent_path() / "mod.json").wstring());
	fout << w.write(setting);
	fout.close();
}


int OverlayInstance::UnInit(ImGuiContext* context)
{
	boost::unique_lock<boost::mutex> l(font_mutex);
	ImGui::SetCurrentContext(context);
	//SaveSettings();
	initialized = false;
	return 0;
}

void OverlayInstance::TextureData(int index, unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
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

void OverlayInstance::SetTexture(int index, void* texture)
{
	boost::unique_lock<boost::mutex> l(font_mutex);
	overlay_texture = texture;
	if (texture == nullptr)
	{
		need_font_init = true;
		// TODO: deinit texture;
	}
}

bool OverlayInstance::GetTextureDirtyRect(int index, int dindex, RECT* rect)
{
	return false;
}

int OverlayInstance::TextureBegin()
{
	return 1; // texture Size and lock
}

void OverlayInstance::TextureEnd()
{
	return;
}

void OverlayInstance::FontsPreferences() 
{
	if (ImGui::TreeNode("Fonts"))
	{
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
			static std::vector<const char *> font_cstr_filenames_prefix = font_cstr_filenames;
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
				const std::string val = boost::to_lower_copy(fonts[index].fontname);
				const std::string val2 = val + ".ttc";
				const std::string val3 = val + ".ttf";
				font_cstr_filenames_prefix.clear();
				for (auto k = font_cstr_filenames.begin(); k != font_cstr_filenames.end(); ++k)
				{
					if (boost::starts_with(boost::to_lower_copy(std::string(*k)), val))
						font_cstr_filenames_prefix.push_back(*k);
				}
				auto fi = std::find_if(font_cstr_filenames.begin(), font_cstr_filenames.end(), [&val, &val2, &val3](const std::string& a) {
					return val == boost::to_lower_copy(a) || val2 == boost::to_lower_copy(a) || val3 == boost::to_lower_copy(a);
				});
				if (fi != font_cstr_filenames.end())
				{
					fontname_idx = fi - font_cstr_filenames.begin();
				}
				else
				{
					fontname_idx = -1;
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
				font_size = font_sizes;
				ImGui::OpenPopup("Append Column");
			}
			ImGui::SameLine();
			if (ImGui::Button("Apply"))
			{
				font_setting_dirty = true;
			}
			if (ImGui::BeginPopup("Append Column"))
			{
				static char buf[100] = { 0, };
				static int glyph_range = 0;
				static int fontname_idx = -1;
				static std::vector<const char *> font_cstr_filenames_prefix = font_cstr_filenames;
				if (ImGui::InputText("FontName", buf, 99))
				{
					std::string val = boost::to_lower_copy(std::string(buf));
					const std::string val2 = val + ".ttc";
					const std::string val3 = val + ".ttf";
					font_cstr_filenames_prefix.clear();
					for (auto k = font_cstr_filenames.begin(); k != font_cstr_filenames.end(); ++k)
					{
						if (boost::starts_with(boost::to_lower_copy(std::string(*k)), val))
							font_cstr_filenames_prefix.push_back(*k);
					}
					auto fi = std::find_if(font_cstr_filenames.begin(), font_cstr_filenames.end(), [&val, &val2, &val3](const std::string& a) {
						return val == boost::to_lower_copy(a) || val2 == boost::to_lower_copy(a) || val3 == boost::to_lower_copy(a);
					});
					if (fi != font_cstr_filenames.end())
					{
						fontname_idx = fi - font_cstr_filenames.begin();
					}
					else
					{
						fontname_idx = -1;
					}
				}
				if (ImGui::ListBox("FontName", &fontname_idx, font_cstr_filenames_prefix.data(), font_cstr_filenames_prefix.size(), 10))
				{
					if (fontname_idx >= 0)
					{
						strcpy(buf, font_cstr_filenames_prefix[fontname_idx]);
					}
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
						SaveSettings();
					}
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine(0,100);
			if (ImGui::Button("Reset"))
			{
				font_sizes = 13;
				fonts = {
					Font("consolab.ttf", "Default", font_sizes),
					Font("Default", "Default", font_sizes), // Default will ignore font size.
					Font("ArialUni.ttf", "Japanese", font_sizes),
					Font("NanumBarunGothic.ttf", "Korean", font_sizes),
					Font("gulim.ttc", "Korean", font_sizes),
				};
			}

			//if (ImGui::BeginPopup("Edit Column"))
			if (index >= 0)
			{
				ImGui::BeginGroup();
				ImGui::Text("Edit");
				ImGui::Separator();
				//font_cstr_filenames;
				if (ImGui::InputText("FontName", buf, 99))
				{
					fonts[index].fontname = buf;
					std::string val = boost::to_lower_copy(fonts[index].fontname);
					const std::string val2 = val + ".ttc";
					const std::string val3 = val + ".ttf";
					font_cstr_filenames_prefix.clear();
					for (auto k = font_cstr_filenames.begin(); k != font_cstr_filenames.end(); ++k)
					{
						if (boost::starts_with(boost::to_lower_copy(std::string(*k)), val))
							font_cstr_filenames_prefix.push_back(*k);
					}
					auto fi = std::find_if(font_cstr_filenames.begin(), font_cstr_filenames.end(), [&val, &val2, &val3](const std::string& a) {
						return val == boost::to_lower_copy(a) || val2 == boost::to_lower_copy(a) || val3 == boost::to_lower_copy(a);
					});
					if (fi != font_cstr_filenames.end())
					{
						fontname_idx = fi - font_cstr_filenames.begin();
					}
					else
					{
						fontname_idx = -1;
					}
					SaveSettings();
				}

				if (ImGui::ListBox("FontName", &fontname_idx, font_cstr_filenames_prefix.data(), font_cstr_filenames_prefix.size(), 10))
				{
					if (fontname_idx >= 0)
					{
						fonts[index].fontname = font_cstr_filenames_prefix[fontname_idx];
						strcpy(buf, font_cstr_filenames_prefix[fontname_idx]);
						SaveSettings();
					}
				}
				if (ImGui::Combo("GlyphRange", &glyph_range, glyph_range_key.data(), glyph_range_key.size()))
				{
					if (glyph_range >= 0)
					{
						fonts[index].glyph_range = glyph_range_key[glyph_range];
						SaveSettings();
					}
				}
				if (ImGui::InputFloat("Size", &font_size, 0.5f))
				{
					font_size = std::min(std::max(font_size, 6.0f), 30.0f);
					fonts[index].font_size = font_size;
					SaveSettings();
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
				SaveSettings();
			}
		}
		ImGui::TreePop();
	}
}

void OverlayInstance::Preference(ImGuiContext* context, bool* show_preferences)
{	
	if(ImGui::Begin("Preferences", show_preferences, windows_default_sizes["Preferences"], -1, ImGuiWindowFlags_NoCollapse))
	{
		ImGui::Text("ACTWebSocketOverlay - %s", VERSION_LONG_STRING);
		windows_default_sizes["Preferences"] = ImGui::GetWindowSize();
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

		if (ImGui::TreeNode("WebSocket", "%s : %s", "WebSocket", websocket_message))
		{
			ImGui::Text("WebSocket : %s://%s:%s%s",
#if defined(USE_SSL)
				websocket_ssl ? "wss" :
#endif
				"ws", websocket_host, websocket_port, websocket_path);
#if defined(USE_SSL)
			if (ImGui::Checkbox("Use SSL", &boolean_map["WebSocketSSL"]))
			{
				websocket_reconnect = true;
				if (websocket)
				{
					websocket->close(boost::beast::websocket::none);
				}
				strcpy_s(websocket_message, 1023, "Connecting...");
				SaveSettings();
			}
#endif	
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
		FontsPreferences();
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
		if (ImGui::TreeNode("Info"))
		{
			ImGui::Text("Version : %s", VERSION_LONG_STRING);
			ImGui::Text("Github : https://github.com/ZCube/ACTWebSocket");
			ImGui::Text("Github : https://github.com/ZCube/ACTWebSocketOverlay");
			ImGui::Text("");
			ImGui::TreePop();
		}
	}
	ImGui::End();
}

int OverlayInstance::Render(ImGuiContext* context)
{
	bool move_key_pressed = GetAsyncKeyState(VK_SHIFT) && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_MENU);
	bool use_input = movable || show_preferences || move_key_pressed;
	try {
		ImGui::SetCurrentContext(context);
		static ImGuiContext* c = context;
		if (c != context)
		{
			LoadSettings(context);
			c = context;
		}

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
			windows_default_sizes["AWIO(ActWebSocket ImGui Overlay)"] = ImGui::GetWindowSize();

			mutex.lock();

			auto p = ImGui::GetCursorPos();
			auto &io = ImGui::GetIO();
			{
				auto cog_img = overlay_images.find("cog");
				auto resize_img = overlay_images.find("resize");
				if (cog_img != overlay_images.end() && resize_img != overlay_images.end())
				{
					double scale = (io.Fonts->Fonts[0]->FontSize* io.FontGlobalScale) / cog_img->second.height / 1.5f;
					ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0, 0, 0, 0));
					float icon_color_change = (cosf(GetTickCount() / 500.0f) + 1.0f) / 2.0f;
					ImVec4 color = ColorWithAlpha(color_map["TitleText"], text_opacity * global_opacity);
					color.x *= icon_color_change;
					color.y *= icon_color_change;
					color.z *= icon_color_change;
					ImGui::BeginChild("Btn",
						//ImVec2(100,100),
						ImVec2((cog_img->second.width + 18) * scale, (cog_img->second.height + 18) * scale),
						false,
						ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
					if (ImGui::ImageButton(overlay_texture, ImVec2(resize_img->second.width * scale, resize_img->second.height * scale), resize_img->second.uv0, resize_img->second.uv1, 2, ImVec4(0, 0, 0, 0), color))
					{
						movable = !movable;
					}
					ImGui::EndChild();
					ImGui::SameLine((cog_img->second.width + 18) * scale);
					ImGui::BeginChild("Btn1",
						//ImVec2(100,100),
						ImVec2((resize_img->second.width + 18) * scale, (cog_img->second.height + 18) * scale),
						false,
						ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
					if (ImGui::ImageButton(overlay_texture, ImVec2(cog_img->second.width * scale, cog_img->second.height * scale), cog_img->second.uv0, cog_img->second.uv1, 2, ImVec4(0, 0, 0, 0), color))
					{
						show_preferences = !show_preferences;
					}
					ImGui::EndChild();
					ImGui::SameLine();
					ImGui::SetCursorPos(p);
					ImGui::PopStyleColor(1);
				}
			}
			ImGui::SameLine();
			ImGui::SetCursorPos(p);

			dealerTable.RenderTable(overlay_texture, &overlay_images);

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
					windows_default_sizes["Status"] = ImGui::GetWindowSize();
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

			typedef std::chrono::duration<long, std::chrono::milliseconds> milliseconds;
			static std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

			// auto save per 60 sec
			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() > 60000) // 60sec
			{
				boost::unique_lock<boost::mutex> l(font_mutex);
				SaveSettings();
				start = end;
			}

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

bool OverlayInstance::Menu(bool* show)
{
	if (show)
		show_preferences = *show;
	else
		show_preferences = !show_preferences;
	return show_preferences;
}

bool OverlayInstance::UpdateFont(ImGuiContext* context)
{
	if (font_setting_dirty)
	{
		bool ret = LoadFonts(context);
		return ret;
	}
	return false;
}

bool OverlayInstance::LoadFonts(ImGuiContext* context)
{
	if (!context)
		return false;

	{
		// Changing or adding fonts at runtime causes a crash.
		// TODO: is it possible ?...

		ImGuiIO& io = context->IO;

		WCHAR result[MAX_PATH] = {};
		GetWindowsDirectoryW(result, MAX_PATH);
		boost::filesystem::path p = result;

		GetModuleFileNameW(NULL, result, MAX_PATH);
		boost::filesystem::path m = result;

		// font
		io.Fonts->Clear();
		io.Fonts->TexID = 0;

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
		font_paths.clear();
		// first time
		if (font_filenames.empty())
		{
			font_filenames.clear();
			font_cstr_filenames.clear();
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
		}

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
							if (j->fontname.empty())
								continue;
							std::vector<std::string> extensions = { "", ".ttc", ".ttf" };
							for (auto l = extensions.begin(); l != extensions.end(); ++l) {
								// ttf, ttc only
								auto fontpath = (*k) / (j->fontname + *l);
								if (boost::filesystem::exists(fontpath) && boost::filesystem::is_regular_file(fontpath))
								{
									io.Fonts->AddFontFromFileTTF((fontpath).string().c_str(), j->font_size, &config, glyph_range_map[j->glyph_range]);
									is_loaded = true;
									config.MergeMode = true;
								}
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
	}
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
	return true;
}

void OverlayInstance::websocketThread()
{
	boost::unique_lock<boost::mutex> l(mutex);
	// only one.
	if (!websocket_thread)
		Run();
}

void OverlayInstance::Process(const std::string& message_str)
{
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
				mutex.unlock();
			}
		}
	}
}
