
#include <drea/core/Core>
#include <algorithm>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );
	
	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
	// what the app rejects, the app reports: drea only owns the misuses it
	// detects itself (see the exit code it returns for those)
	int		exitCode = drea::core::toInt( drea::core::ExitCode::Ok );

	app.commander().run( [ &app, &exitCode ]( const std::string & cmd ){
		app.logger().debug( "command to run {}", cmd );

		bool reverse = app.config().get<bool>( "reverse" );
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
			exitCode = drea::core::toInt( drea::core::ExitCode::UsageError );
		}
	});
	return exitCode;
}
