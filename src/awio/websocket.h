/*
* This file is subject to the terms and conditions defined in
* file 'LICENSE', which is part of this source code package.
*/

#pragma once
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#if defined(USE_SSL)
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ssl.hpp>
#endif

class websocket_context_base
{
public:
	boost::asio::io_service ios;
	boost::asio::ip::tcp::socket sock{ ios };
	boost::asio::io_service::strand strand{ ios };
	virtual void handshake(const std::string& host_port, const std::string& path) = 0;
	virtual void async_read(boost::beast::multi_buffer& dynabuf, std::function<void(boost::beast::error_code, std::size_t)> && handler) = 0;
	virtual void async_write(boost::asio::const_buffers_1& dynabuf, std::function<void(boost::beast::error_code, std::size_t)>&& handler) = 0;
	virtual void write(boost::asio::const_buffers_1& dynabuf) = 0;
	virtual void write_post(boost::asio::const_buffers_1& dynabuf)
	{
		strand.post(std::bind(&websocket_context_base::write, this, dynabuf));
	}
	virtual void close(const boost::beast::websocket::close_reason& reason) = 0;
};
#if defined(USE_SSL)
class websocket_ssl_context : public websocket_context_base
{
public:
	websocket_ssl_context()
	{
		stream.set_verify_mode(boost::asio::ssl::verify_none);
	}
	boost::asio::ssl::context ctx{ boost::asio::ssl::context::sslv23 };
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket&> stream{ sock, ctx };
	boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>&> ws{ stream };

	virtual void handshake(const std::string& host_port, const std::string& path)
	{
		stream.handshake(boost::asio::ssl::stream_base::client);

		//boost::beast::websocket::permessage_deflate pmd;
		//pmd.client_enable = true;
		//pmd.server_enable = true;
		//pmd.server_no_context_takeover = true;
		//pmd.client_no_context_takeover = true;
		//pmd.compLevel = 3;

		ws.auto_fragment(false);
		//ws.set_option(pmd);

		ws.handshake(host_port, path);
	}
	virtual void async_read(boost::beast::multi_buffer& dynabuf, std::function<void(boost::beast::error_code, std::size_t)> && handler)
	{
		ws.async_read(dynabuf, handler);
	}

	virtual void async_write(boost::asio::const_buffers_1& dynabuf, std::function<void(boost::beast::error_code, std::size_t)>&& handler)
	{
		ws.async_write(dynabuf, handler);
	}
	virtual void write(boost::asio::const_buffers_1& dynabuf)
	{
		ws.write(dynabuf);
	}
	virtual void close(const boost::beast::websocket::close_reason& reason)
	{
		strand.post([this, reason]() {
			ws.close(reason);
		});
	}
};
#endif

class websocket_context : public websocket_context_base
{
public:
	boost::beast::websocket::stream<boost::asio::ip::tcp::socket&> ws{ sock };

	virtual void handshake(const std::string& host_port, const std::string& path)
	{
		//boost::beast::websocket::permessage_deflate pmd;
		//pmd.client_enable = true;
		//pmd.server_enable = true;
		//pmd.server_no_context_takeover = true;
		//pmd.client_no_context_takeover = true;
		//pmd.compLevel = 3;

		ws.auto_fragment(false);
		//ws.set_option(pmd);

		ws.handshake(host_port, path);
	}
	virtual void async_read(boost::beast::multi_buffer& dynabuf, std::function<void(boost::beast::error_code, std::size_t)> && handler)
	{
		ws.async_read(dynabuf, handler);
	}

	virtual void async_write(boost::asio::const_buffers_1& dynabuf, std::function<void(boost::beast::error_code, std::size_t)>&& handler)
	{
		ws.async_write(dynabuf, handler);
	}
	virtual void write(boost::asio::const_buffers_1& dynabuf)
	{
		ws.write(dynabuf);
	}
	virtual void close(const boost::beast::websocket::close_reason& reason)
	{
		strand.post([this, reason]() {
			ws.close(reason);
		});
	}
};
