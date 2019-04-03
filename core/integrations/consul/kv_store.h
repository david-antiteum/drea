#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <chrono>

#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>

namespace drea { namespace core { namespace integrations { namespace Consul {

class KVStore
{
public:
	KVStore( const std::string & consulHost )
	{
		mConsulService = fmt::format( "{}/v1/kv", consulHost );
	}

	std::string get( const std::string & key ) const
	{
		web::http::client::http_client 	client( utility::conversions::to_string_t( mConsulService + "/" + key ) );
		web::http::http_request			req( web::http::methods::GET );
		std::string						res;

		req.headers().set_content_type( utility::conversions::to_string_t( "application/json; charset=utf-8" ));

		client.request( req ).then([this, key](web::http::http_response response){
			if( response.status_code() == web::http::status_codes::OK ){
				return response.extract_json();
			}else{
				spdlog::error( "Error accessing consul {} for {}", response.status_code(), mConsulService + "/" + key );
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
