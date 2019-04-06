
#include <drea/core/Core>
#include <algorithm>

int main( int argc, char * argv[] )
{
	drea::core::App	 app;

	app.setName( "say" );
	app.setDescription( "An example for the Drea Framework.\n\nDrea is available at https://github.com/david-antiteum/drea." );
	app.setVersion( "0.0.1" );
	
	app.config().addDefaults().add(
		{
			"reverse", "", "reverse string"
		}
	);
	app.commander().addDefaults().add(
		{
			"say", "prints the argument", {}, { "reverse" }
		}
	);
	app.parse( argc, argv );
	app.commander().run( [ &app ]( std::string cmd ){
		if( cmd == "say" ){
			if( app.commander().arguments().size() == 1 ){
				std::string say = app.commander().arguments().front();
				if( app.config().contains( "reverse" ) ){
					std::reverse( say.begin(), say.end() );
				}
				app.logger()->info( "{}", say );
			}
		}else{
			app.commander().reportNoCommand( cmd );
		}
	});
}
