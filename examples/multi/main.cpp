
#include <drea/core/Core>
#include <algorithm>
#include <boost/algorithm/string.hpp>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );
	
	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
	app.commander().run( [ &app ]( const std::string & ){
		for( std::string value: app.config().getAll<std::string>( "env" ) ){
			std::vector<std::string>		entries;
			boost::algorithm::split( entries, value, [](char ch){ return ch == '='; } );

			if( entries.size() == 2 ){
				fmt::print( "{}={}\n", entries.at(0), entries.at(1) );
			}
		}
	});
}
