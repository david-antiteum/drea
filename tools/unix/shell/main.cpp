
#include <drea/core/Core>
#include <iostream>
#include <fstream>
#include <sstream>

#include "commands.yml.h"

std::string readFile( const std::string & configFileName )
{
	// TODO ... use  std::filesystem::exists( configFileName )
	std::string		res;
	std::ifstream 	configFile;

	configFile.open( configFileName.c_str(), std::ios::in );
	if( configFile.is_open() ){
		std::stringstream buffer;
		buffer << configFile.rdbuf();
		res = buffer.str();
		if( res.empty() ){
			spdlog::warn( "The config file {} is empty", configFileName );
		}
	}else{
		spdlog::error( "Cannot read the config file {}", configFileName );
	}
	return res;
}

void generateMan( const drea::core::App & app, const std::string & yamlFileName, const std::string & manFileName )
{
	char * argv[] = { "app" };

	drea::core::App	 _app( 1, argv );

	_app.parse( readFile( yamlFileName ) );

	// https://liw.fi/manpages/
}

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );
	
	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
	app.commander().run( [ &argc, &argv, &app ]( std::string cmd ){
		if( cmd == "man" ){
			if( app.commander().arguments().size() == 2 ){
				generateMan( app, app.commander().arguments().at( 0 ), app.commander().arguments().at( 1 ) );
			}else{
				app.commander().wrongNumberOfArguments( cmd );
			}
		}else{
			app.commander().unknownCommand( cmd );
		}
	});
}
