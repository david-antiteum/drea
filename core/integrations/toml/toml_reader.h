#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <sstream> 

#include <cpptoml.h>

#include "App.h"
#include "Config.h"

namespace drea { namespace core { namespace integration { namespace toml {

static bool valid( const std::string & val )
{
	bool	res = false;

	try{
		std::stringstream  stream;
		
		stream << val;
		auto config = cpptoml::parser( stream ).parse();
		res = true;
	}catch(...){

	}
	return res;
}

static std::string asString( std::shared_ptr<cpptoml::base> value )
{
	std::string		res;

	if( dynamic_cast< cpptoml::value<double> * >( value.get() )){
		res = fmt::format( "{}", dynamic_cast< cpptoml::value<double> * >( value.get() )->get() );
	}else if( dynamic_cast< cpptoml::value<int64_t> * >( value.get() )){
		res = fmt::format( "{}", dynamic_cast< cpptoml::value<int64_t> * >( value.get() )->get() );
	}else if( dynamic_cast< cpptoml::value<std::string> * >( value.get() )){
		res =dynamic_cast< cpptoml::value<std::string> * >( value.get() )->get();
	}else if( dynamic_cast< cpptoml::value<bool> * >( value.get() )){
		if( dynamic_cast< cpptoml::value<bool> * >( value.get() )->get() ){
			res = "true";
		}else{
			res ="false";
		}
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

static void readConfig( const App & app, std::shared_ptr<cpptoml::table> config, const std::string & prefix )
{
	for( auto & [key, value]: *config ){
		auto realKey = addPrefix( prefix, key );

		if( dynamic_cast< cpptoml::table * >( value.get() )){
			readConfig( app, value->as_table(), realKey );
		}else if( dynamic_cast< cpptoml::table_array * >( value.get() )){
			for( auto table: *dynamic_cast< cpptoml::table_array * >( value.get() ) ){
				readConfig( app, table->as_table(), realKey );
			}
		}else{
			if( auto option = app.config().find( realKey ) ){
				app.config().registerUse( realKey );
				if( !option->mParamName.empty() ){
					if( dynamic_cast< cpptoml::array *>( value.get() )){
						option->mValues.clear();
						for( auto seqVal: dynamic_cast< cpptoml::array *>( value.get() )->get() ){
							app.config().append( option->mName, asString( seqVal ) );
						}
					}else{
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

static void readConfig( const App & app, const std::string & val )
{
	if( !val.empty() ){
		try{
			std::stringstream  stream;
		
			stream << val;

			readConfig( app, cpptoml::parser( stream ).parse(), {} );
		}catch( const std::runtime_error & e ){
			spdlog::error( "Error parsing a configuration in YAML. Error: {}", e.what() );
		}
	}
}

}}}}
