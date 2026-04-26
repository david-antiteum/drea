#include <drea/core/Core>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
	drea::core::App app( argc, argv );

	app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );

	// Resolve the caller's "tier" from a CLI flag for demo purposes. A real
	// app would derive this from a session token, an env var, an LDAP lookup,
	// a license file, etc. The CLI flag keeps the example self-contained.
	const std::string tier = app.config().used( "tier" )
		? app.config().get<std::string>( "tier" )
		: std::string{};

	std::vector<std::string> groups;
	if( tier == "readonly" || tier == "operator" ){
		groups.push_back( "staff-readonly" );
	}
	if( tier == "operator" ){
		groups.push_back( "staff-operator" );
	}
	app.commander().setEnabledGroups( groups );

	// Anonymous footer: appended only to top-level --help when no tier is
	// active. Per-command help and authenticated callers do not see it.
	const bool anonymous = !app.config().used( "tier" );
	app.setHelpFooter( [anonymous]( const drea::core::App &, std::string_view command ){
		if( anonymous && command.empty() ){
			return std::string( "More commands available with --tier readonly|operator." );
		}
		return std::string{};
	});

	app.commander().run( [ &app ]( const std::string & cmd ){
		if( cmd == "ping" ){
			app.logger().info( "pong" );
		}else if( cmd == "cost.top" ){
			app.logger().info( "cost.top: ran" );
		}else if( cmd == "cost.pipeline-status" ){
			app.logger().info( "cost.pipeline-status: ran" );
		}else if( cmd == "workflow.send" ){
			app.logger().info( "workflow.send: ran" );
		}else if( cmd == "workflow.list" ){
			app.logger().info( "workflow.list: ran" );
		}else if( cmd == "rotate-keys" ){
			app.logger().info( "rotate-keys: ran" );
		}else{
			app.commander().unknownCommand( cmd );
		}
	});
}
