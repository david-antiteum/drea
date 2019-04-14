#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <yaml-cpp/yaml.h>

#include "App.h"
#include "Config.h"

namespace drea { namespace core { namespace integration { namespace yaml {

static bool valid( const std::string & val )
{
	bool	res = false;

	try{
		auto node = YAML::Load( val );
		res = true;
	}catch(...){

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

static void readConfig( const App & app, const YAML::Node & config, const std::string & prefix )
{
	for( auto node: config ){
		auto realKey = addPrefix( prefix, node.first.as<std::string>() );

		if( node.second.Type() == YAML::NodeType::Map ){
			readConfig( app, node.second, realKey );
		}else{
			if( auto option = app.config().find( realKey ) ){
				app.config().registerUse( realKey );
				if( !option->mParamName.empty() ){
					// discard data: config file and defaults
					if( node.second.Type() == YAML::NodeType::Scalar ){
						app.config().set( option->mName, node.second.as<std::string>() );
					}else if( node.second.Type() == YAML::NodeType::Sequence ){
						option->mValues.clear();
						for( auto seqVal: node.second ){
							if( seqVal.Type() == YAML::NodeType::Scalar ){
								app.config().append( option->mName, seqVal.as<std::string>() );
							}
						}
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
			readConfig( app, YAML::Load( val ), {} );
		}catch( const std::runtime_error & e ){
			spdlog::error( "Error parsing a configuration in YAML. Error: {}", e.what() );
		}
	}
}

}}}}
