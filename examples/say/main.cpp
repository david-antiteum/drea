
#include <drea/core/Core>
#include <algorithm>

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );

	app.setName( "say" );
	app.setDescription( "An example for the Drea Framework.\n\nDrea is available at https://github.com/david-antiteum/drea." );
	app.setVersion( "0.0.1" );
	
	app.config().addDefaults();
	app.config().add(
		{
			"reverse", "", "reverse string"
		}
	);
	app.commander().addDefaults();
	auto say = app.commander().add(
		{
			"say", "prints the argument", {}, { "reverse" }
		}
	);
	auto repeat = app.commander().add(
		{
			"repeat", "repeat something", {}, { "reverse" }, {}, { "parrot" }
		}
	);
	auto parrot = app.commander().add(
		{
			"parrot", "print parrot", {}, { "reverse" }, "repeat", { "blue", "red" }
		}
	);
	auto blue = app.commander().add(
		{
			"blue", "print a blue parrot", {}, { "reverse" }, "parrot"
		}
	);
	auto red = app.commander().add(
		{
			"red", "print a red parrot", {}, { "reverse" }, "parrot"
		}
	);
	app.parse();
	app.commander().run( [ &app ]( std::string cmd ){
		app.logger()->debug( "command to run {}", cmd );
		if( cmd == "say" ){
			if( !app.commander().arguments().empty() ){
				bool reverse = app.config().used( "reverse" );
				for( auto say: app.commander().arguments() ){
					if( reverse ){
						std::reverse( say.begin(), say.end() );
					}
					app.logger()->info( "{}", say );
				}
			}
		}else if( cmd == "repeat" ){
			app.commander().reportNoSubCommand( cmd );
		}else if( cmd == "repeat.parrot" ){
			app.commander().reportNoSubCommand( cmd );
		}else if( cmd == "repeat.parrot.blue" ){
			if( app.config().used( "reverse" ) ){
				app.logger()->info( "{}", "eulb torrap" );
			}else{
				app.logger()->info( "{}", "parrot blue" );
			}
		}else if( cmd == "repeat.parrot.red" ){
			if( app.config().used( "reverse" ) ){
				app.logger()->info( "{}", "der torrap" );
			}else{
				app.logger()->info( "{}", "parrot red" );
			}
		}else{
			app.commander().reportNoCommand( cmd );
		}
	});
}
