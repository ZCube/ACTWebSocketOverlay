/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include "version.h"
#include <imgui.h>
#include "imgui_internal.h"
#include <imgui_lua_bindings.cpp>
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
#include <boost/thread/shared_mutex.hpp>
#include <chrono>

#include <atlstr.h>
#define STR2UTF8(s) (CW2A(CA2W(s), CP_UTF8))

static char websocket_message[1024] = { 0, };
static char websocket_host[256] = { 0, };
static char websocket_port[256] = { 0, };
static bool websocket_reconnect = true;

static void* overlay_texture = nullptr;
static unsigned char *overlay_texture_filedata = nullptr;
static int overlay_texture_width = 0, overlay_texture_height = 0, overlay_texture_channels = 0;

#include <concurrent_queue.h>
std::string data;
boost::mutex data_mutex;

std::string processed_data;
boost::mutex processed_data_mutex;
Concurrency::concurrent_queue<std::string> data_queue;

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

					beast::websocket::stream<boost::asio::ip::tcp::socket&> ws{ sock };

					std::string host_port = host + ":" + websocket_port;
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
							data_queue.push(message_str);
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
};

static WebSocket websocket;

inline static void websocketThread()
{
	// only one.
	if (!websocket.websocket_thread)
		websocket.Run();
}


class OverlayInstance
{
public:

	lua_State *L;
	OverlayInstance()
	{
		char buff[256];
		int error;
		L = luaL_newstate();   /* opens Lua */
		lState = L;
		LoadImguiBindings();
		luaopen_base(L);             /* opens the basic library */
		luaopen_table(L);            /* opens the table library */
		luaopen_io(L);               /* opens the I/O library */
		luaopen_string(L);           /* opens the string lib. */
		luaopen_math(L);             /* opens the math lib. */
	}
	~OverlayInstance()
	{
	}

	void BuildScript(const std::string& logic_script, const std::string& render_script)
	{
	}

	void Run()
	{
		std::string data;
		typedef std::chrono::duration<long, std::chrono::milliseconds> milliseconds;
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
		while (true)
		{
			while (data_queue.try_pop(data))
			{
				{
					boost::mutex::scoped_lock l(data_mutex);
					::data = data;
				}
			}
			Sleep(50);
		}
	}
};
boost::mutex m;
std::shared_ptr<OverlayInstance> instance;


class Serializable
{
public:
	virtual void ToJson(Json::Value& value) const {};
	virtual void FromJson(Json::Value& value) {};
};

typedef union
{
	struct
	{
		unsigned char Blue;
		unsigned char Green;
		unsigned char Red;
		unsigned char Alpha;
	} rgba;

	float float_value;
	uint32_t long_value;
} RGBValue;
class Window;
class DrawCommand : public Serializable
{
public:
	DrawCommand() {}
	DrawCommand(const std::string& cmd_, const std::string& text_, ImVec2 min_, ImVec2 max_, ImVec4 color_)
	{
		cmd = cmd_;
		text = text_;
		min = min_;
		max = max_;
		color.rgba.Blue = color_.x * 255;
		color.rgba.Green = color_.y * 255;
		color.rgba.Red = color_.z * 255;
		color.rgba.Alpha = color_.w * 255;
	}
	DrawCommand(const std::string& cmd_, ImVec2 min_, ImVec2 max_, ImVec4 color_)
	{
		cmd = cmd_;
		min = min_;
		max = max_;
		color.rgba.Blue = color_.x * 255;
		color.rgba.Green = color_.y * 255;
		color.rgba.Red = color_.z * 255;
		color.rgba.Alpha = color_.w * 255;
	}
	DrawCommand(const std::string& cmd_, ImVec2 min_, ImVec2 max_)
	{
		cmd = cmd_;
		min = min_;
		max = max_;
	}
	std::string cmd;
	std::string text;
	ImVec2 min, max;
	RGBValue color;
	virtual void Draw(class Window* win) const {
		static std::unordered_map<std::string, std::function<void(const DrawCommand&)> > draws =
		{
			{ "None", [](const DrawCommand& dc) {
		} },
		{ "Text", [](const DrawCommand& dc) {
			{
				ImVec2 p = ImGui::GetWindowPos();
				ImVec2 mi(dc.min.x + p.x, dc.min.y + p.y);
				ImVec2 ma(dc.max.x + p.x, dc.max.y + p.y);
				ImGui::GetWindowDrawList()->AddText(mi, dc.color.long_value, dc.text.c_str());
			}
		} },
		{ "Image", [](const DrawCommand& dc) {
			auto im = overlay_images.find(dc.text);
			if (im != overlay_images.end())
			{
				ImVec2 p = ImGui::GetWindowPos();
				ImVec2 mi(dc.min.x + p.x, dc.min.y + p.y);
				ImVec2 ma(dc.max.x + p.x, dc.max.y + p.y);
				ImGui::GetWindowDrawList()->AddImage(overlay_texture, mi, ma, im->second.uv0, im->second.uv1, dc.color.long_value);
			}
		} },
		{ "PushClipRect", [](const DrawCommand& dc) {
			{
				ImVec2 p = ImGui::GetWindowPos();
				ImVec2 mi(dc.min.x + p.x, dc.min.y + p.y);
				ImVec2 ma(dc.max.x + p.x, dc.max.y + p.y);
				ImGui::GetWindowDrawList()->PushClipRect(mi, ma);
			}
		} },
		{ "PopClipRect", [](const DrawCommand& dc) {
			ImGui::GetWindowDrawList()->PopClipRect();
		} },
		{ "Rect", [](const DrawCommand& dc) {
			{
				ImVec2 p = ImGui::GetWindowPos();
				ImVec2 mi(dc.min.x + p.x, dc.min.y + p.y);
				ImVec2 ma(dc.max.x + p.x, dc.max.y + p.y);
				ImGui::GetWindowDrawList()->AddRect(mi, ma, dc.color.long_value);
			}
		} },
		{ "RectFill", [](const DrawCommand& dc) {
			{
				ImVec2 p = ImGui::GetWindowPos();
				ImVec2 mi(dc.min.x + p.x, dc.min.y + p.y);
				ImVec2 ma(dc.max.x + p.x, dc.max.y + p.y);
				ImGui::GetWindowDrawList()->AddRectFilled(mi, ma, dc.color.long_value);
			}
		} },
		};

		auto& draw = draws.find(cmd);
		if (draw != draws.end())
		{
			draw->second(*this);
		}
	};
	virtual void ToJson(Json::Value& value) const {
		value["cmd"] = cmd;
		value["text"] = text;
		value["min_x"] = min.x;
		value["min_y"] = min.y;
		value["max_x"] = min.x;
		value["max_y"] = min.y;
		std::string ret;
		value["cr"] = color.rgba.Red;
		value["cg"] = color.rgba.Green;
		value["cb"] = color.rgba.Blue;
		value["ca"] = color.rgba.Alpha;
	};
	virtual void FromJson(Json::Value& value) {
		cmd = value["cmd"].asString();
		text = value["text"].asString();
		min = ImVec2(value["min_x"].asFloat(), value["min_y"].asFloat());
		max = ImVec2(value["max_x"].asFloat(), value["max_y"].asFloat());
		color.rgba.Red = value["cr"].asInt();
		color.rgba.Green = value["cg"].asInt();
		color.rgba.Blue = value["cb"].asInt();
		color.rgba.Alpha = value["ca"].asInt();
	};
};

class Window : public Serializable
{
public:
	std::string name;
	ImVec2 min, max;
	int clipStack;

	std::vector<DrawCommand> commands;
	void Begin()
	{
		clipStack = 0;
	}
	void End()
	{
		if (ImGui::GetWindowDrawList())
		{
			while (clipStack--)
			{
				ImGui::GetWindowDrawList()->PopClipRect();
			}
		}
	}
	void Draw()
	{
		for (auto cmd = commands.begin(); cmd != commands.end(); ++cmd)
		{
			cmd->Draw(this);
		}
	}
	virtual void ToJson(Json::Value& value) const {
		value.clear();
		auto& cmds = value["cmds"];
		for (auto i = commands.begin(); i != commands.end(); ++i)
		{
			Json::Value v;
			i->ToJson(v);
			cmds.append(v);
		}
	};
	virtual void FromJson(Json::Value& value) {
		if (!value.empty())
		{
			int idx = 0;
			auto& cmds = value["cmds"];
			commands.resize(cmds.size());
			for (auto i = cmds.begin(); i != cmds.end(); ++i, ++idx)
			{
				commands[idx].FromJson((*i));
			}
		}
		else
		{
			commands.clear();
		}
	};
};
class UI : public Serializable
{
public:
	std::map<std::string, Window> windows;
	virtual void ToJson(Json::Value& value) const {
		value.clear();
		for (auto i = windows.begin(); i != windows.end(); ++i)
		{
			i->second.ToJson(value[i->first]);
		}
	};
	virtual void FromJson(Json::Value& value) {
		//windows.clear();
		for (auto i = value.begin(); i != value.end(); ++i)
		{
			windows[i.key().asString()].FromJson(*i);
		}
	};
};

static boost::shared_mutex ui_mutex;
static UI ui;


extern "C" int ModInit(ImGuiContext* context)
{
	ImGui::SetCurrentContext(context);
	ImGui::GetIO().Fonts->AddFontDefault();

	{
		boost::unique_lock<boost::mutex> l(m);
		if (!instance)
		{
			instance = std::make_shared<OverlayInstance>();
		}
		new std::thread(boost::bind(&OverlayInstance::Run, instance));// , instance->Run();

		strcpy(websocket_port, "10501");
		strcpy(websocket_host, "127.0.0.1");

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
				Json::Value overlay_atlas;;
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
	}

	// datathread;
	// render;
	websocketThread();
	return 0;
}

extern "C" int ModUnInit(ImGuiContext* context)
{
	try {
	}
	catch (...)
	{

	}
	return 0;
}
extern "C" void ModTextureData(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
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
	overlay_texture = texture;
	if (texture == nullptr)
	{
	}
}

extern "C" int ModRender(ImGuiContext* context)
{
	std::string logic, render;
	wchar_t szPath[1024] = { 0, };
	GetModuleFileNameW(NULL, szPath, 1023);
	boost::filesystem::path p(szPath);
	{
		auto L = instance->L;
		p = p.parent_path() / "test.lua";
		int status = luaL_loadfile(instance->L, p.string().c_str());

		if (status) {
			/* If something went wrong, error message is at the top of */
			/* the stack */
			fprintf(stderr, "Couldn't load file: %s\n", lua_tostring(instance->L, -1));
			exit(1);
		}
		int result = lua_pcall(instance->L, 0, LUA_MULTRET, 0);
		printf("%d\r\n", result);
		if (result) {
			fprintf(stderr, "Failed to run script: %s\n", lua_tostring(L, -1));
		}
	}
	return 0;
}
