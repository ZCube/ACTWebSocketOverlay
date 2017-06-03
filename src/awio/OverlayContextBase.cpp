/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#include "OverlayContextBase.h"
#include <boost/algorithm/hex.hpp>
#include <atlstr.h>
#define STR2UTF8(s) (CW2A(CA2W(s), CP_UTF8))
#define UTF82WSTR(s) (CA2W(s), CP_UTF8)

void OverlayContextBase::Process(const std::string & message_str) {}

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
					beast::multi_buffer b;
					beast::websocket::opcode op;
					//std::function<void(std::string write) > write;
					std::function<void(beast::error_code) > read_handler =
						[this, &read_handler, &b, &op](beast::error_code ec) {
						if (ec || websocket_reconnect)
						{
							strcpy_s(websocket_message, 1023, "Disconnected");
						}
						else
						{
							std::string message_str;
							message_str = boost::lexical_cast<std::string>(beast::buffers(b.data()));
							// debug
							//std::cout << beast::buffers(b.data()) << "\n";
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
								websocket->async_read(op, b, websocket->strand.wrap(read_handler));
						}
					};
					if (loop)
						websocket->async_read(op, b, websocket->strand.wrap(read_handler));
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

