#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <chrono>
#include <iomanip>

#include "utilities/httpclient.h"

namespace drea { namespace core { namespace integrations { namespace etcd {

/*! Access KV storer using the old v2 HTTP API.
	Todo:
	- move to v3 gRPC API
	- use secure access
*/
class KVStore
{
public:
	KVStore( const std::string & etcdHost )
	{
		mEtcdService = fmt::format( "{}/v2/keys", etcdHost );
	}

	std::string get( const std::string & key ) const
	{
		std::string		address = mEtcdService + "/" + key;
		std::string		res;

		spdlog::info( "accesing to etcd KV store for {}", address );

		auto jsonMaybe = utilities::HttpClient::get( address );
		if( jsonMaybe ){
			auto jsonValue = jsonMaybe.value();
			try{
				res = jsonValue.at( "node" ).at( "value" ).get<std::string>();
			}catch( const std::exception & e ){
				spdlog::error( "{}. Json was: {}", e.what(), jsonValue.dump() );
			}
		}
		return res;
	}

private:
	std::string			mEtcdService;
};

}}}}
