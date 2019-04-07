#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

#include <optional>

#include <cstdlib>
#include <iostream>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <nlohmann/json.hpp>

using tcp = boost::asio::ip::tcp; // from <boost/asio.hpp>
namespace http = boost::beast::http; // from <beast/http.hpp>

namespace drea { namespace core { namespace utilities { namespace httpclient {

std::string _get( const std::string & address )
{
	std::string					res;
	boost::beast::error_code 	ec;

	// Set up an asio socket
	boost::asio::io_service ios;
	tcp::socket sock{ ios };

	// Make the connection on the IP address we get from a lookup
	std::string host = "127.0.0.1";
	unsigned short port = 2379;
	tcp::endpoint mdendpoint( boost::asio::ip::address(boost::asio::ip::address::from_string( host )), port );
	sock.connect( mdendpoint, ec );

	// Set up an HTTP GET request message
	http::request<http::string_body> req;
	req.method(http::verb::get);
	req.target("/v2/keys/calculator-key");
	req.set(http::field::host, host + ":" + std::to_string(port));
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	req.prepare_payload();

	http::write(sock, req, ec);
	boost::beast::flat_buffer b;

	http::response<http::string_body> body;
	http::read(sock, b, body, ec);

	res = body.body();

	sock.shutdown(tcp::socket::shutdown_both, ec);

	return res;
}

std::optional<nlohmann::json> get( const std::string & address )
{
	std::optional<nlohmann::json>	res;
	const std::string				body = _get( address );

	if( !body.empty() ){
		res = body;
	}
	return res;
}

}}}}
