#include <drea/core/Core>
#include <algorithm>

int main( int argc, char * argv[] )
{
    drea::core::App     app( argc, argv );

    app.setName( "say" );
    app.setDescription( "Prints the argument of the command \"this\" and quits." );
    app.setVersion( "0.0.1" );

    app.config().addDefaults().add(
        {
            "reverse", "", "reverse string"
        }
    );
    app.commander().addDefaults().add(
        {
            "this", "string", "prints the argument", { "reverse" }
        }
    );
    app.parse();
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
