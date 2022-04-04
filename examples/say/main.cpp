
#include <drea/core/Core>
#include <algorithm>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );
	
	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
	app.commander().run( [ &app ]( const std::string & cmd ){
		app.logger().debug( "command to run {}", cmd );

		bool reverse = app.config().used( "reverse" );
		if( cmd == "this" ){
			for( auto say: app.commander().arguments() ){
				if( reverse ){
					std::reverse( say.begin(), say.end() );
				}
				app.logger().info( "{}", say );
			}
		}else if( cmd == "repeat.parrot.blue" ){
			if( reverse ){
				app.logger().info( "{}", "eulb torrap" );
			}else{
				app.logger().info( "{}", "parrot blue" );
			}
		}else if( cmd == "repeat.parrot.red" ){
			if( reverse ){
				app.logger().info( "{}", "der torrap" );
			}else{
				app.logger().info( "{}", "parrot red" );
			}
		}else{
			app.commander().unknownCommand( cmd );
		}
	});
}
