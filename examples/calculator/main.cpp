
#include <drea/core/Core>

#include <numeric>
#include <exception>
#include <optional>
#include <iostream>

#include "commands.yml.h"
#include "version.yml.h"

double sum( const drea::core::App & app )
{
	double sum = 0.0;

	for( const auto & arg: app.commander().arguments() ){
		try{
			sum += std::stod( arg );
		}catch( const std::exception & e ){
			app.logger().error( "Argument {} is not a number: {}", arg, e.what() );
		}
	}
	return sum;
}

std::optional<double> power( const drea::core::App & app )
{
	std::optional<double>	res;

	try{
		double 	base = std::stod( app.commander().arguments().at( 0 ) );
		double 	exponent = std::stod( app.commander().arguments().at( 1 ) );

		res = std::pow( base, exponent );
	}catch( const std::exception & e ){
		app.logger().error( "Argument is not a number: {}", e.what() );
	}
	return res;
}

size_t count( const drea::core::App & app )
{
	size_t sum = 0;

	for( const auto & arg: app.commander().arguments() ){
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

	app.addToParser( std::string( version_yml, version_yml + version_yml_len ) );
	app.addToParser( std::string( commands_yml, commands_yml + commands_yml_len ) );
	app.parse();
	app.commander().run( [ &app ]( const std::string & cmd ){
		app.logger().debug( "Run called for command {}", cmd );

		std::optional<double>		valueMaybe;

		if( cmd == "sum"){
			if( !app.commander().arguments().empty() ){
				valueMaybe = sum( app );
			}else{
				app.commander().wrongNumberOfArguments( cmd );
			}
		}else if( cmd == "power" ){
			if( app.commander().arguments().size() == 2 ){		
				valueMaybe = power( app );
			}else{
				app.commander().wrongNumberOfArguments( cmd );
			}
		}else if( cmd == "count" ){
			if( !app.commander().arguments().empty() ){
				valueMaybe = static_cast<double>( count( app ) );
			}else{
				app.commander().wrongNumberOfArguments( cmd );
			}
		}else{
			app.commander().unknownCommand( cmd );
		}
		if( valueMaybe ){
			writeResult( app, valueMaybe.value() );
		}
	});
}
