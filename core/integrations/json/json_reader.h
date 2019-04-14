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

static std::string asString( const nlohmann::json & value )
{
	std::string		res;

	if( value.is_number_float() ){
		res = fmt::format( "{}", value.get<float>() );
	}else if( value.is_string() ){
		res = value.get<std::string>();
	}else if( value.is_boolean() ){
		if( value.get<bool>() ){
			res = "true";
		}else{
			res = "false";
		}
	}else if( value.is_number() ){
		res = fmt::format( "{}", value.get<int>() );
	}
	return res;
}

static std::string addPrefix( const std::string & prefix, const std::string & name )
{
	if( prefix.empty() ){
		return name;
	}else{
		return prefix + "." + name;
	}
}

static void readConfig( const App & app, const nlohmann::json & config, const std::string & prefix )
{
	if( config.is_object() ){
		for( auto & [key, value]: config.items() ){
			auto realKey = addPrefix( prefix, key );

			if( value.is_object() ){
				readConfig( app, value, realKey );
			}else{
				if( auto option = app.config().find( realKey ) ){
					app.config().registerUse( realKey );
					if( !option->mParamName.empty() ){
						if( value.is_array() ){
							option->mValues.clear();
							for( auto seqVal: value ){
								if( seqVal.is_primitive() ){
									app.config().append( option->mName, asString( seqVal ) );
								}
							}
						}else if( value.is_primitive() ){
							app.config().set( option->mName, asString( value ) );
						}
						if( option->mValues.empty() ){
							spdlog::warn( "Missing arguments for flag {}", realKey );
						}
					}
				}else{
					app.config().reportUnknownArgument( realKey );
				}
			}
		}
	}
}

static void readConfig( const App & app, const std::string & val )
{
	if( !val.empty() ){
		try{
			readConfig( app, nlohmann::json::parse( val ), {} );
		}catch( const std::runtime_error & e ){
			spdlog::error( "Error parsing a configuration in YAML. Error: {}", e.what() );
		}
	}
}

}}}}
