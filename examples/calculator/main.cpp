
#include <drea/core/Core>

#include <numeric>
#include <exception>
#include <optional>
#include <iostream>

#include "commands.yml.h"

double sum( const drea::core::App & app )
{
	double sum = 0.0;

	for( auto arg: app.commander().arguments() ){
		try{
			sum += std::stod( arg );
		}catch( std::exception & e ){
			app.logger().error( "Argument {} is not a number: {}", arg, e.what() );
		}
	}
	return sum;
}

std::optional<double> power( const drea::core::App & app )
{
	std::optional<double>	res;

	if( app.commander().arguments().size() == 2 ){
		try{
			double 	base = std::stod( app.commander().arguments().at( 0 ) );
			double 	exponent = std::stod( app.commander().arguments().at( 1 ) );

			res = std::pow( base, exponent );
		}catch( std::exception & e ){
			app.logger().error( "Argument is not a number: {}", e.what() );
		}
	}else{
		app.logger().error( "Power needs two arguments" );
	}
	return res;
}

size_t count( const drea::core::App & app )
{
	size_t sum = 0;

	for( auto arg: app.commander().arguments() ){
		sum += arg.size();
	}
	return sum;
}

void writeResult( const drea::core::App & app, double value )
{
	if( app.config().used( "round" ) ){
		value = std::round( value );
	}
	app.logger().info( "Result: {}", value );

	if( app.config().used( "equal" ) ){
		if( app.config().get<double>( "equal" ) == value ){
			app.logger().info( "Result is equal" );
		}else{
			app.logger().info( "Result is different than {}", app.config().get<double>( "equal" ) );
		}
	}
}

int main( int argc, char * argv[] )
{
	drea::core::App	 app( argc, argv );

	app.config().addDefaults();
	app.commander().addDefaults();

	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
	app.commander().run( [ &app ]( std::string cmd ){
		app.logger().debug( "Run called for command {}", cmd );

		std::optional<double>		valueMaybe;

		if( cmd == "sum"){
			valueMaybe = sum( app );
		}else if( cmd == "power" ){
			valueMaybe = power( app );
		}else if( cmd == "count" ){
			valueMaybe = static_cast<double>( count( app ) );
		}else{
			app.commander().unknownCommand( cmd );
		}
		if( valueMaybe ){
			writeResult( app, valueMaybe.value() );
		}
	});
}
