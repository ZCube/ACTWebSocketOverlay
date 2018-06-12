/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include "OverlayContextBase.h"
#include <boost/algorithm/hex.hpp>
#include <boost/lexical_cast.hpp>
#include <atlstr.h>
#define STR2UTF8(s) (CW2A(CA2W(s), CP_UTF8))
#define UTF82WSTR(s) (CA2W(s), CP_UTF8)

void OverlayContextBase::WebSocketCheck()
{
	if (current_websocket_port != websocket_port ||
#ifdef USE_SSL
		current_websocket_ssl != websocket_ssl ||
#endif
		current_websocket_host != websocket_host ||
		current_websocket_path != websocket_path
		)
	{
		websocket_reconnect = true;
	}
}

void OverlayContextBase::Process(const std::string & message_str) {}

OverlayContextBase::OverlayContextBase() {
	websocket_ssl = false;
	websocket_host = "localhost";
	websocket_port = 0;
	websocket_path = "/";
}

boost::filesystem::path OverlayContextBase::GetImagesPath() {
	return root_path / "images";
}

void OverlayContextBase::WebSocketRun()
{
	auto func = [this]()
	{
		while (loop)
		{
			try {
				websocket_reconnect = false;

				char buf[32] = { 0, };
				sprintf(buf, "%d", websocket_port);

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
					current_websocket_host = websocket_host;
					current_websocket_port = websocket_port;
					current_websocket_ssl = websocket_ssl;
					current_websocket_path = websocket_path;
					boost::beast::multi_buffer b;
					boost::beast::websocket::detail::opcode op;
					//std::function<void(std::string write) > write;
					std::function<void(boost::beast::error_code, std::size_t) > read_handler =
						[this, &read_handler, &b, &op](boost::beast::error_code ec, std::size_t bytes_transferred) {
						if (ec || websocket_reconnect)
						{
							strcpy_s(websocket_message, 1023, "Disconnected");
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

void OverlayContextBase::WebSocketStop()
{
	loop = false;
	if (websocket_thread)
	{
		websocket_thread->join();
		websocket_thread.reset();
	}
}

void OverlayContextBase::Preferences()
{
	ImGui::PushID(name.c_str());
	if (ImGui::TreeNode(name.c_str(), "%s : %s", name.c_str(), websocket_message))
	{
		ImGui::Separator();
		if (ImGui::TreeNode("WebSocket", "WebSocket : %s://%s:%d%s",
#if defined(USE_SSL)
			websocket_ssl ? "wss" :
#endif
			"ws", websocket_host.c_str(), websocket_port, websocket_path.c_str()))
		{
#if defined(USE_SSL)
			if (ImGui::Checkbox("Use SSL", &websocket_ssl))
			{
				websocket_reconnect = true;
				if (websocket)
				{
					websocket->close(boost::beast::websocket::none);
				}
				strcpy_s(websocket_message, 1023, "Connecting...");
				Save();
			}
#endif
			{
				websocket_host.reserve(65);
				if (ImGui::InputText("Host", (char*)websocket_host.data(), 64))
				{
					int len = strnlen_s(websocket_host.data(), 64);
					websocket_host.resize(len);

					websocket_reconnect = true;
					if (websocket)
					{
						websocket->close(boost::beast::websocket::none);
					}
					strcpy_s(websocket_message, 1023, "Connecting...");
					Save();
				}
			}
			{
				websocket_path.reserve(1024);
				websocket_path[0] = '/';
				if (ImGui::InputText("Path", (char*)websocket_path.data() + 1, 1022))
				{
					int len = strnlen_s(websocket_path.data() + 1, 1022);
					websocket_path.resize(len + 1);

					websocket_reconnect = true;
					if (websocket)
					{
						websocket->close(boost::beast::websocket::none);
					}
					strcpy_s(websocket_message, 1023, "Connecting...");
					Save();
				}
			}
			if (ImGui::InputInt("Port", &websocket_port))
			{
				websocket_reconnect = true;
				if (websocket)
				{
					websocket->close(boost::beast::websocket::none);
				}
				strcpy_s(websocket_message, 1023, "Connecting...");
				Save();
			}
			ImGui::TreePop();
		}
		PreferencesBody();
		ImGui::Separator();
		ImGui::TreePop();
	}
	ImGui::PopID();
}

void OverlayContextBase::PreferencesBody()
{
	for (auto i = preference_nodes.begin(); i != preference_nodes.end(); ++i)
	{
		PreferenceBase::Preferences(*i, preference_storage);
	}
}

