#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <chrono>

#include "utilities/httpclient.h"
#include <boost/beast/core/detail/base64.hpp>

namespace drea::core::integrations::Consul {

/*! Access KV storer using the HTTP API.
	Todo:
	- use secure access
	- read encrypted data
*/
class KVStore
{
public:
	explicit KVStore( const std::string & consulHost )
	{
		mConsulService = fmt::format( "{}/v1/kv", consulHost );
	}

	std::string get( const std::string & key ) const
	{
		std::string		address = mConsulService + "/" + key;
		std::string		res;

		spdlog::info( "accesing to consul KV store for {}", address );

		auto jsonMaybe = utilities::HttpClient::get( address );
		if( jsonMaybe ){
			auto jsonValue = jsonMaybe.value();
			try{
				res = jsonValue[0].at( "Value" ).get<std::string>();
			}catch( const std::exception & e ){
				spdlog::error( "{}. Json was: {}", e.what(), jsonValue.dump() );
			}
			if( !res.empty() ){
				res = boost::beast::detail::base64_decode( res );
			}
		}
		return res;
	}

private:
	std::string			mConsulService;
};

}
