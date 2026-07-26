
#include <drea/core/Core>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );

	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
	app.commander().run( [ &app ]( std::string /*cmd*/ ){
		// the app takes root params (see commands.yml): they arrive as
		// arguments with no command given
		if( auto names = app.commander().arguments(); names.empty() ){
			app.logger().info( "World!" );
		}else{
			for( const auto & name: names ){
				app.logger().info( "{}!", name );
			}
		}
	});
}
