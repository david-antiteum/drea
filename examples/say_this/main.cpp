#include <drea/core/Core>
#include <algorithm>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
    drea::core::App     app( argc, argv );

	app.config().addDefaults();
	app.commander().addDefaults();
	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
    app.commander().run( [ &app ]( std::string cmd ){
        if( cmd == "this" && app.commander().arguments().size() == 1 ){
            std::string say = app.commander().arguments().front();
            if( app.config().used( "reverse" ) ){
                std::reverse( say.begin(), say.end() );
            }
            app.logger().info( "{}", say );
        }
    });
}
