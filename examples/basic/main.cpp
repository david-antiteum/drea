
#include "core/App.h"
#include "core/Config.h"
#include "core/Commander.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <numeric>
#include <exception>

int main( int argc, char * argv[] )
{
    drea::core::App     app;

	app.setName( "basic" );
    app.setDescription( "An app example." );
    app.setVersion( "0.0.1" );
    
	app.config().addDefaults().add(
		{
			"round", "", "round the result to the nearest integer"
		}
	).add(
		{
			"equal", "number", "check if the result is equal to <number>"
		}
	);

    app.commander().addDefaults().add(
        {
            "sum", "sum <args>"
        }
    );

	app.parse( argc, argv );

	app.commander().run( [ &app ]( std::string cmd ){
		app.logger()->debug( "Run called for command {}", cmd );
		if( cmd == "sum"){
			double sum = 0.0;

			for( auto arg: app.commander().arguments() ){
				try{
					sum += std::stod( arg );
				}catch( std::exception & e ){
					app.logger()->error( "Argument {} is not a number: {}", arg, e.what() );
				}
			}
			if( app.config().contains( "round" ) ){
				sum = std::round( sum );
			}
			app.logger()->info( "SUM: {}", sum );

			if( app.config().contains( "equal" ) ){
				try{
					if( std::stod( app.config().value( "equal") ) == sum ){
						app.logger()->info( "RES is EQUAL" );
					}else{
						app.logger()->info( "RES is DIFFERENT" );
					}
				}catch( std::exception & e ){
					app.logger()->error( "Argument {} is not a number: {}", app.config().value( "equal"), e.what() );
				}
			}
		}
	});
}
