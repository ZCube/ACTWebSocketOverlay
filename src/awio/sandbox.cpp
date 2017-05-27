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
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>

#include <libplatform/libplatform.h>
#include <v8pp/module.hpp>

#include <atlstr.h>
#define STR2UTF8(s) (CW2A(CA2W(s), CP_UTF8))

#include <v8.h>

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
	if (!websocket.websocket_thread)
		websocket.Run();
}



class V8Instance
{
public:
	v8::Platform* platform;
	v8::Isolate::CreateParams create_params;
	v8::Isolate::CreateParams create_params2;

	V8Instance()
	{
	}
	~V8Instance()
	{
	}

	static v8::Local<v8::String> StringToV8String(const std::string& str)
	{
		v8::Local<v8::String> ret = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), str.data());
		return ret;
	}

	static std::string V8StringToString(v8::Local<v8::Value> value)
	{
		return std::string(*v8::String::Utf8Value(value));
	}

	static void messageGetter(v8::Local<v8::String> property,
		const v8::PropertyCallbackInfo<v8::Value>& info) {
		boost::mutex::scoped_lock l(data_mutex);
		info.GetReturnValue().Set(StringToV8String(::data));
	}

	void Run()
	{
		using namespace v8;
		v8::V8::Initialize();
		char modulePath[MAX_PATH+1] = { 0, };
		GetModuleFileNameA(NULL, modulePath, MAX_PATH);

		v8::V8::InitializeICUDefaultLocation(modulePath);
		v8::V8::InitializeExternalStartupData(modulePath);
		platform = v8::platform::CreateDefaultPlatform();
		v8::V8::InitializePlatform(platform);
		v8::V8::Initialize();

		create_params.array_buffer_allocator =
			v8::ArrayBuffer::Allocator::NewDefaultAllocator();
		using namespace v8;
		Isolate* isolate = Isolate::New(create_params);
		{
			// Create a stack-allocated handle scope.
			HandleScope handle_scope(isolate);

			Local<ObjectTemplate> global = ObjectTemplate::New(isolate);

			global->SetInternalFieldCount(1);
			global->SetAccessor(String::NewFromUtf8(isolate, "data"), &V8Instance::messageGetter);

			Isolate::Scope isolate_scope(isolate);

			Local<Context> context = Context::New(isolate, NULL, global);

			Context::Scope context_scope(context);
			
			Local<String> source =
				String::NewFromUtf8(isolate, "'Hello' + ', World!' + data",
					NewStringType::kNormal).ToLocalChecked();

			// Compile the source code.
			Local<Script> script = Script::Compile(context, source).ToLocalChecked();
			Local<UnboundScript> us = script->GetUnboundScript();

			std::string data;
			while(true)
			{
				while (data_queue.try_pop(data))
				{
					{
						boost::mutex::scoped_lock l(data_mutex);
						::data = data;
					}
					Local<Value> result = script->Run(context).ToLocalChecked();
					if (!result.IsEmpty() && result->IsString())
					{
						boost::mutex::scoped_lock l(processed_data_mutex);
						processed_data = V8StringToString(result);
						std::cerr << processed_data << std::endl;
					}
				}

				Sleep(50);
			}
		}

		isolate->Dispose();
		v8::V8::Dispose();
		v8::V8::ShutdownPlatform();
		delete platform;
		delete create_params.array_buffer_allocator;
	}
};
boost::mutex m;
std::shared_ptr<V8Instance> instance;


extern "C" int ModInit(ImGuiContext* context)
{
	ImGui::SetCurrentContext(context);
	ImGui::GetIO().Fonts->AddFontDefault();

	{
		boost::unique_lock<boost::mutex> l(m);
		if (!instance)
		{
			instance = std::make_shared<V8Instance>();
		}
		new std::thread(boost::bind(&V8Instance::Run, instance));// , instance->Run();

		strcpy(websocket_port, "10501");
		strcpy(websocket_host, "127.0.0.1");
	}

	// datathread;
	// render;

	websocketThread();
	return 0;
}

extern "C" int ModUnInit(ImGuiContext* context)
{
	return 0;
}

extern "C" void ModTextureData(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
}

extern "C" void ModSetTexture(void* texture)
{
}

extern "C" int ModRender(ImGuiContext* context)
{
	return 0;
}
