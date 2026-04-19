
#include <drea/core/Core>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
	drea::core::App app( argc, argv );

	// "internal" is added programmatically. It is not declared in commands.yml.
	drea::core::Command internal;
	internal.mName = "internal";
	internal.mDescription = "engineering-only diagnostic";
	app.commander().add( internal );

	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );

	// Runtime hiding: flip Command::mHidden based on any condition the app knows
	// about. Here, "--dev" unhides developer-only commands. In real apps this
	// could be an env var, a license check, a build-time constant, a role from
	// an auth token, etc.
	//
	// Note: hiding after parse() only affects --help rendering and bash
	// autocompletion. "Did you mean?" suggestions on typos are emitted during
	// parse(), so they still consider these commands. If hiding must also
	// affect those errors, set mHidden before calling parse() using a
	// condition that doesn't depend on CLI flags (env var, build constant...).
	const bool devMode = app.config().used( "dev" );
	if( !devMode ){
		if( auto cmd = app.commander().find( "debug" ) ){
			cmd->mHidden = true;
		}
		if( auto cmd = app.commander().find( "internal" ) ){
			cmd->mHidden = true;
		}
	}

	app.commander().run( [ &app ]( const std::string & cmd ){
		if( cmd == "ping" ){
			app.logger().info( "pong" );
		}else if( cmd == "debug" ){
			app.logger().info( "debug: ran (hidden from --help unless --dev)" );
		}else if( cmd == "internal" ){
			app.logger().info( "internal: ran (hidden from --help unless --dev)" );
		}else{
			app.commander().unknownCommand( cmd );
		}
	});
}
