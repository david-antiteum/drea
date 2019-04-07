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

#include "utilities/uri.h"

using tcp = boost::asio::ip::tcp; // from <boost/asio.hpp>
namespace http = boost::beast::http; // from <beast/http.hpp>

namespace drea { namespace core { namespace utilities { namespace httpclient {

std::string _get( const std::string & address )
{
	std::string					res;
	Uri 						uri( address );
	boost::beast::error_code 	ec;

	if( !uri.isValid() ){
		spdlog::error( "cannot parse uri {}", address );
		return res;
	}
	boost::asio::io_service ios;
	tcp::socket sock{ ios };

	tcp::endpoint endpoint( boost::asio::ip::address::from_string( uri.host() ), uri.port() );
	sock.connect( endpoint, ec );
	if( ec ){
		spdlog::error( "Error conecting to {}. {}", address, ec.message() );
	}else{
		http::request<http::string_body> req;
		req.method( http::verb::get );
		req.target( uri.path() );
		req.set( http::field::host, uri.host() + ":" + std::to_string( uri.port() ) );
		req.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
		req.prepare_payload();

		http::write(sock, req, ec);
		if( ec ){
			spdlog::error( "Error sending request to {}. {}", address, ec.message() );
		}else{
			boost::beast::flat_buffer b;

			http::response<http::string_body> body;
			http::read(sock, b, body, ec);
			if( ec ){
				spdlog::error( "Error reading from {}. {}", address, ec.message() );
			}else{
				res = body.body();
			}
		}
		sock.shutdown(tcp::socket::shutdown_both, ec);
	}
	return res;
}

std::optional<nlohmann::json> get( const std::string & address )
{
	std::optional<nlohmann::json>	res;
	const std::string				body = _get( address );

	if( !body.empty() ){
		try{
			res = nlohmann::json::parse( body );
		}catch(...){
			// TODO report parsing error
		}
	}else{
		// TODO report empty 
	}
	return res;
}

std::string _post( const std::string & address, const std::string & value )
{
	std::string					res;
	Uri 						uri( address );
	boost::beast::error_code 	ec;

	if( !uri.isValid() ){
		spdlog::error( "cannot parse uri {}", address );
		return res;
	}
	boost::asio::io_service ios;
	tcp::socket sock{ ios };

	tcp::endpoint endpoint( boost::asio::ip::address::from_string( uri.host() ), uri.port() );
	sock.connect( endpoint, ec );
	if( ec ){
		spdlog::error( "Error conecting to {}. {}", address, ec.message() );
	}else{
		http::request<http::string_body> req;
		req.method( http::verb::post );
		req.target( uri.path() );
		req.set( http::field::host, uri.host() + ":" + std::to_string( uri.port() ) );
		req.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
		req.body() = value;
		req.prepare_payload();

		http::write(sock, req, ec);
		if( ec ){
			spdlog::error( "Error sending request to {}. {}", address, ec.message() );
		}else{
			boost::beast::flat_buffer b;

			http::response<http::string_body> body;
			http::read(sock, b, body, ec);
			if( ec ){
				spdlog::error( "Error reading from {}. {}", address, ec.message() );
			}else{
				res = body.body();
			}
		}
		sock.shutdown(tcp::socket::shutdown_both, ec);
	}
	return res;
}

void post( const std::string & address, const nlohmann::json & json )
{
	_post( address, json.dump() );
}

}}}}
