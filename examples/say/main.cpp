
#include <drea/core/Core>
#include <algorithm>

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );

	app.setName( "say" );
	app.setDescription( "An example for the Drea Framework.\nDrea is available at https://github.com/david-antiteum/drea." );
	app.setVersion( "0.0.1" );
	
	app.config().addDefaults().add(
		{
			"reverse", "", "reverse string"
		}
	);
	app.commander().addDefaults().add({
		{
			"this", "string", "prints the argument", {}, { "reverse" }
		},
		{
			"repeat", "", "repeat something", {}, { "reverse" },
		},
		{
			"parrot", "", "print parrot", {}, { "reverse" }, "repeat"
		},
		{
			"blue", "", "print a blue parrot", {}, { "reverse" }, "repeat.parrot"
		},
		{
			"red", "", "print a red parrot", {}, { "reverse" }, "repeat.parrot"
		}
	});
	app.parse();
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
