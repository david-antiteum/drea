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

static void readConfig( const App & app, const YAML::Node & config )
{
	for( auto node: config ){
		std::string arg = node.first.as<std::string>();
		if( auto option = app.config().find( arg ) ){
			app.config().registerUse( arg );
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
					spdlog::warn( "Missing arguments for flag {}", arg );
				}
			}
		}else{
			app.config().reportUnknownArgument( arg );
		}
	}
}

static void readConfig( const App & app, const std::string & val )
{
	if( !val.empty() ){
		try{
			readConfig( app, YAML::Load( val ) );
		}catch( const std::runtime_error & e ){
			spdlog::error( "Error parsing a configuration in YAML. Error: {}", e.what() );
		}
	}
}

}}}}
