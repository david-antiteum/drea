
#include <drea/core/Core>
#include <algorithm>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );
	
	app.config().addDefaults();
	app.commander().addDefaults();
	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
	app.commander().run( [ &app ]( std::string cmd ){
		app.logger().debug( "command to run {}", cmd );
		if( cmd == "this" ){
			if( !app.commander().arguments().empty() ){
				bool reverse = app.config().used( "reverse" );
				for( auto say: app.commander().arguments() ){
					if( reverse ){
						std::reverse( say.begin(), say.end() );
					}
					app.logger().info( "{}", say );
				}
			}
		}else if( cmd == "repeat" ){
			app.commander().reportNoSubCommand( cmd );
		}else if( cmd == "repeat.parrot" ){
			app.commander().reportNoSubCommand( cmd );
		}else if( cmd == "repeat.parrot.blue" ){
			if( app.config().used( "reverse" ) ){
				app.logger().info( "{}", "eulb torrap" );
			}else{
				app.logger().info( "{}", "parrot blue" );
			}
		}else if( cmd == "repeat.parrot.red" ){
			if( app.config().used( "reverse" ) ){
				app.logger().info( "{}", "der torrap" );
			}else{
				app.logger().info( "{}", "parrot red" );
			}
		}else{
			app.commander().reportNoCommand( cmd );
		}
	});
}
