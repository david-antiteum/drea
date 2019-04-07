#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <chrono>

#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>

namespace drea { namespace core { namespace integrations { namespace Consul {

/*! Access KV storer using the HTTP API.
	Todo:
	- use secure access
	- read encrypted data
*/
class KVStore
{
public:
	KVStore( const std::string & consulHost )
	{
		mConsulService = fmt::format( "{}/v1/kv", consulHost );
	}

	std::string get( const std::string & key ) const
	{
		std::string						address = mConsulService + "/" + key;
		web::http::client::http_client 	client( utility::conversions::to_string_t( address ) );
		web::http::http_request			req( web::http::methods::GET );
		std::string						res;

		spdlog::info( "accesing to consul KV store for {}", address );

		req.headers().set_content_type( utility::conversions::to_string_t( "application/json; charset=utf-8" ));

		client.request( req ).then([this, address](web::http::http_response response){
			if( response.status_code() == web::http::status_codes::OK ){
				return response.extract_json();
			}else{
				spdlog::error( "Error accessing consul {} for {}", response.status_code(), address );
				return pplx::task_from_result(web::json::value());
			}
		}).then([ &res, this ](pplx::task<web::json::value> previousTask){
			try{
				const auto jsonRes = previousTask.get().as_array();

				if( jsonRes.size() > 0 ){
					auto obj = jsonRes.at(0).as_object();
				
					auto encodedRes = obj.at( utility::conversions::to_string_t( "Value" )).as_string();

					if( !encodedRes.empty() ){
						auto f64 = utility::conversions::from_base64( encodedRes );
						std::string str(f64.begin(), f64.end());

						res = str;
					}
				}
			}catch( const web::http::http_exception & e ){
				spdlog::error( "Error accessing consul {}", e.what() );
			}catch(...){
				spdlog::error( "Error accessing consul" );
			}
		}).wait();

		return res;
	}

private:
	std::string			mConsulService;
};

}}}}
