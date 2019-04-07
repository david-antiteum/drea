#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <chrono>

#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>

namespace drea { namespace core { namespace integrations { namespace etcd {

class KVStore
{
public:
	KVStore( const std::string & etcdHost )
	{
		mEtcdService = fmt::format( "{}/v2/keys", etcdHost );
	}

	std::string get( const std::string & key ) const
	{
		std::string						address = mEtcdService + "/" + key;
		web::http::client::http_client 	client( utility::conversions::to_string_t( address ) );
		web::http::http_request			req( web::http::methods::GET );
		std::string						res;

		spdlog::info( "accesing to etcd KV store for {}", address );

		req.headers().set_content_type( utility::conversions::to_string_t( "application/json; charset=utf-8" ));

		client.request( req ).then([this, address](web::http::http_response response){
			if( response.status_code() == web::http::status_codes::OK ){
				return response.extract_json();
			}else{
				spdlog::error( "Error accessing etcd {} for {}", response.status_code(), address );
				return pplx::task_from_result(web::json::value());
			}
		}).then([ &res, this, address ](pplx::task<web::json::value> previousTask){
			try{
				auto obj = previousTask.get().as_object().at( utility::conversions::to_string_t( "node" )).as_object();
				auto encodedRes = obj.at( utility::conversions::to_string_t( "value" )).as_string();

				if( !encodedRes.empty() ){
					res = utility::conversions::to_utf8string( encodedRes );
				}
			}catch( const web::http::http_exception & e ){
				spdlog::error( "Error accessing etcd {} at {}", e.what(), address );
			}catch(...){
				spdlog::error( "Error accessing etcd at {}", address );
			}
		}).wait();

		return res;
	}

private:
	std::string			mEtcdService;
};

}}}}
