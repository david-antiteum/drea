
#include <drea/core/Core>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );
	
	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
	app.commander().run( [ &app ]( std::string /*cmd*/ ){
		app.logger().info( "World!" );
	});
}
