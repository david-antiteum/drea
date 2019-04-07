
#include <drea/core/Core>

#include <numeric>
#include <exception>
#include <optional>
#include <iostream>

int main( int argc, char * argv[] )
{
	drea::core::App	 app;

	app.setName( "calculator" );
	app.setDescription( "A basic calculator as an example for the Drea Framework.\n\nDrea is available at https://github.com/david-antiteum/drea." );
	app.setVersion( "0.0.1" );
	
	app.config().setEnvPrefix( "CAL" );
	app.config().addRemoteProvider( "consul", "http://127.0.0.1:8500", "calculator-key" );
	app.config().addRemoteProvider( "etcd", "http://127.0.0.1:2379", "calculator-key" );
	app.config().addDefaults();
	app.config().add(
		{
			"round", "", "round the result to the nearest integer"
		}
	);
	app.config().add(
		{
			"equal", "number", "check if the result is equal to <number>", {}, typeid( double )
		}
	);

	app.commander().addDefaults();
	app.commander().add(
		{
			"sum", "sum all the arguments", {}, { "round", "equal" }
		}
	);
	app.commander().add(
		{
			"power", "raise the first argument to the power of the second", {}, { "round", "equal" }
		}		
	);
	app.commander().add(
		{
			"count", "count the characters of a text argument", {}, { "equal" }
		}		
	);

	app.parse( argc, argv );
	app.commander().run( [ &app ]( std::string cmd ){
		app.logger()->debug( "Run called for command {}", cmd );

		std::optional<double>		valueMaybe;

		if( cmd == "sum"){
			double sum = 0.0;

			for( auto arg: app.commander().arguments() ){
				try{
					sum += std::stod( arg );
				}catch( std::exception & e ){
					app.logger()->error( "Argument {} is not a number: {}", arg, e.what() );
				}
			}
			valueMaybe = sum;
		}else if( cmd == "power" ){
			if( app.commander().arguments().size() == 2 ){
				try{
					double 	base = std::stod( app.commander().arguments().at( 0 ) );
					double 	exponent = std::stod( app.commander().arguments().at( 1 ) );

					valueMaybe = std::pow( base, exponent );
				}catch( std::exception & e ){
					app.logger()->error( "Argument is not a number: {}", e.what() );
				}
			}else{
				app.logger()->error( "Power needs two arguments" );
			}
		}else if( cmd == "count" ){
			double sum = 0.0;

			for( auto arg: app.commander().arguments() ){
				sum += arg.size();
			}
			valueMaybe = sum;
		}else{
			app.commander().reportNoCommand( cmd );
		}
		if( valueMaybe ){
			double	value = valueMaybe.value();
			if( app.config().used( "round" ) ){
				value = std::round( value );
			}
			app.logger()->info( "Result: {}", value );

			if( app.config().used( "equal" ) ){
				if( app.config().get<double>( "equal" ) == value ){
					app.logger()->info( "Result is equal" );
				}else{
					app.logger()->info( "Result is different than {}", app.config().get<double>( "equal" ) );
				}
			}
		}
	});
}
