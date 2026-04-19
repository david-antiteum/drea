
#include <drea/core/Core>

#include <string>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
	drea::core::App app( argc, argv );

	app.config().add({
		{ "db.host",     "host",     "Database host",                     {},     typeid( std::string ) },
		{ "db.port",     "port",     "Database port",                     { 5432 }, typeid( int ) },
		{ "db.user",     "user",     "Database user",                     {},     typeid( std::string ) }
	});

	drea::core::Option pw;
	pw.mName = "db.password";
	pw.mParamName = "password";
	pw.mDescription = "Database password (from secret)";
	pw.mType = typeid( std::string );
	pw.mSensitive = true;
	app.config().add( pw );

	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
	app.commander().run( [ &app ]( std::string /*cmd*/ ){
		auto & log = app.logger();

		const std::string password = app.config().get<std::string>( "db.password" );
		const std::string masked = password.empty()
			? std::string( "<unset>" )
			: std::string( password.size(), '*' );

		log.info( "Resolved configuration:" );
		log.info( "  db.host     = {}", app.config().get<std::string>( "db.host" ) );
		log.info( "  db.port     = {}", app.config().get<int>( "db.port" ) );
		log.info( "  db.user     = {}", app.config().get<std::string>( "db.user" ) );
		log.info( "  db.password = {} ({} chars)", masked, password.size() );
	});
}
