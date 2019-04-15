
#include <drea/core/Core>

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );

	app.setName( "hello" );
	app.setDescription( "An example for the Drea Framework.\nDrea is available at https://github.com/david-antiteum/drea." );
	app.setVersion( "0.0.1" );
	
	app.config().addDefaults();
	app.commander().addDefaults();
	app.parse();
	app.commander().run( [ &app ]( std::string cmd ){
		app.logger().info( "World!" );
	});
}
