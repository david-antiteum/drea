#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <nlohmann/json.hpp>

#include "App.h"
#include "Config.h"

namespace drea { namespace core { namespace integration { namespace json {

static bool valid( const std::string & val )
{
	bool	res = false;

	try{
		auto json = nlohmann::json::parse( val );
		res = true;
	}catch(...){

	}
	return res;
}

static void readConfig( const App & app, const nlohmann::json & config )
{
	if( config.is_object() ){
		for( auto & [key, value]: config.items() ){
			if( auto option = app.config().find( key ) ){
				app.config().registerUse( key );
				if( !option->mParamName.empty() ){
					// discard data: config file and defaults
					if( value.is_number_float() ){
						app.config().set( option->mName, fmt::format( "{}", value.get<float>() ) );
					}else if( value.is_string() ){
						app.config().set( option->mName, value.get<std::string>() );
					}else if( value.is_boolean() ){
						if( value.get<bool>() ){
							app.config().set( option->mName, "true" );
						}else{
							app.config().set( option->mName, "false" );
						}
					}else if( value.is_array() ){
						option->mValues.clear();
						for( auto seqVal: value ){
							if( seqVal.is_number_float() ){
								app.config().append( option->mName, fmt::format( "{}", seqVal.get<float>() ) );
							}
						}
					}
					if( option->mValues.empty() ){
						spdlog::warn( "Missing arguments for flag {}", key );
					}
				}
			}else{
				app.config().reportUnknownArgument( key );
			}
		}
	}
}

static void readConfig( const App & app, const std::string & val )
{
	if( !val.empty() ){
		try{
			readConfig( app, nlohmann::json::parse( val ) );
		}catch( const std::runtime_error & e ){
			spdlog::error( "Error parsing a configuration in YAML. Error: {}", e.what() );
		}
	}
}

}}}}
