#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

#include <optional>
#include <tuple>

#include <cstdlib>
#include <iostream>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <nlohmann/json.hpp>
#include <tl/expected.hpp>

#include "utilities/uri.h"

using tcp = boost::asio::ip::tcp; // from <boost/asio.hpp>
namespace http = boost::beast::http; // from <beast/http.hpp>

namespace drea::core::utilities {

class HttpClient
{
public:
	[[nodiscard]] static std::optional<nlohmann::json> get( std::string_view address )
	{
		return executeJson( address, {}, http::verb::get );
	}

	[[nodiscard]] static std::optional<nlohmann::json> post( std::string_view address, const nlohmann::json & json )
	{
		return executeJson( address, json, http::verb::post );
	}

	[[nodiscard]] static std::optional<nlohmann::json> put( std::string_view address, const nlohmann::json & json )
	{
		return executeJson( address, json, http::verb::put );
	}

private:
	static std::optional<nlohmann::json> executeJson( std::string_view address, const nlohmann::json & json, http::verb verb )
	{
		if( const auto response = execute( address, json.empty() ? "" : json.dump(), verb ); response ){
			try{
				return nlohmann::json::parse( response.value());
			} catch( const std::exception & e ) {
				spdlog::error( "HttpClient error: {}. Response was: {}", e.what(), response.value() );
			}
		}else{
			spdlog::error( "HttpClient error: {}", response.error() );
		}
		return {};
	}

	static tl::expected<std::string,http::status> execute( std::string_view address, std::string_view value, http::verb verb )
	{
		tl::expected<std::string,http::status>	res;

		if( Uri uri( address ); !uri.isValid() ){
			spdlog::error( "cannot parse uri {}", address );
			res = tl::make_unexpected( http::status::bad_request );
		}else{
			boost::asio::io_service 		ios;
			tcp::socket 					sock{ ios };
			boost::beast::error_code 		ecConnect;

			tcp::endpoint endpoint( boost::asio::ip::address::from_string( uri.host() ), uri.port() );
			sock.connect( endpoint, ecConnect );
			if( ecConnect ){
				spdlog::error( "Error conecting to {}. {}", address, ecConnect.message() );
				res = tl::make_unexpected( http::status::service_unavailable );
			}else{
				http::request<http::string_body> req;
				req.method( verb );

				if( auto query = uri.query(); query.empty() ){
					req.target( uri.path() );
				}else{
					req.target( uri.path() + "?" + query );
				}
				req.set( http::field::host, uri.host() + ":" + std::to_string( uri.port() ) );
				req.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
				if( !value.empty() ){
					req.body() = value;
				}
				req.prepare_payload();

				boost::beast::error_code 	ecWrite;
				http::write( sock, req, ecWrite );
				if( ecWrite ){
					spdlog::error( "Error sending request to {}. {}", address, ecWrite.message() );
					res = tl::make_unexpected( http::status::service_unavailable );
				}else{
					boost::beast::flat_buffer 			b;
					http::response<http::string_body> 	response;
					boost::beast::error_code 			ecRead;

					http::read( sock, b, response, ecRead );
					if( ecRead ){
						spdlog::error( "Error reading from {}. {}", address, ecRead.message() );
						res = tl::make_unexpected( http::status::service_unavailable );
					}else{
						res = response.body();
					}
				}
				boost::beast::error_code 		ecShutdown;
				sock.shutdown(tcp::socket::shutdown_both, ecShutdown);
			}
		}
		return res;
	}
};

}
