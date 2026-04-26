#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Commander.h>
#include <drea/core/Command.h>
#include <drea/core/Config.h>


using drea::core::App;
using drea::core::Command;

namespace {

struct AppFixture {
	char  argv0[16] = "drea-test";
	char* argv[1]   = { argv0 };
	App   app;
	AppFixture() : app( 1, argv ) {}
};

}

TEST_CASE( "Command with no groups is always visible", "[groups]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "ping";
	auto added = fx.app.commander().add( cmd );

	REQUIRE( fx.app.commander().isVisible( *added ) );

	fx.app.commander().setEnabledGroups( { "any" } );
	REQUIRE( fx.app.commander().isVisible( *added ) );
}

TEST_CASE( "Command with groups is hidden until a group is enabled", "[groups]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "secret";
	cmd.mGroups = { "staff" };
	auto added = fx.app.commander().add( cmd );

	REQUIRE_FALSE( fx.app.commander().isVisible( *added ) );

	fx.app.commander().setEnabledGroups( { "other" } );
	REQUIRE_FALSE( fx.app.commander().isVisible( *added ) );

	fx.app.commander().setEnabledGroups( { "staff" } );
	REQUIRE( fx.app.commander().isVisible( *added ) );
}

TEST_CASE( "Multi-group command becomes visible when any group enabled", "[groups]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "either";
	cmd.mGroups = { "alpha", "beta" };
	auto added = fx.app.commander().add( cmd );

	fx.app.commander().setEnabledGroups( { "beta" } );
	REQUIRE( fx.app.commander().isVisible( *added ) );

	fx.app.commander().setEnabledGroups( { "alpha", "gamma" } );
	REQUIRE( fx.app.commander().isVisible( *added ) );
}

TEST_CASE( "mHidden overrides groups regardless of enabled set", "[groups]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "force-hidden";
	cmd.mGroups = { "staff" };
	cmd.mHidden = true;
	auto added = fx.app.commander().add( cmd );

	fx.app.commander().setEnabledGroups( { "staff" } );
	REQUIRE_FALSE( fx.app.commander().isVisible( *added ) );
}

TEST_CASE( "Subcommand inherits parent groups when none declared", "[groups]" )
{
	AppFixture fx;
	Command parent;
	parent.mName = "cost";
	parent.mGroups = { "staff" };
	fx.app.commander().add( parent );

	Command child;
	child.mName = "top";
	child.mParentCommand = "cost";
	fx.app.commander().add( child );

	fx.app.commander().configure( {} );

	auto resolved = fx.app.commander().find( "cost.top" );
	REQUIRE( resolved );
	REQUIRE( resolved->mGroups == std::vector<std::string>{ "staff" } );
}

TEST_CASE( "Subcommand keeps own groups, ignoring parent's", "[groups]" )
{
	AppFixture fx;
	Command parent;
	parent.mName = "cost";
	parent.mGroups = { "staff" };
	fx.app.commander().add( parent );

	Command child;
	child.mName = "leaf";
	child.mParentCommand = "cost";
	child.mGroups = { "beta" };
	fx.app.commander().add( child );

	fx.app.commander().configure( {} );

	auto resolved = fx.app.commander().find( "cost.leaf" );
	REQUIRE( resolved );
	REQUIRE( resolved->mGroups == std::vector<std::string>{ "beta" } );
}

TEST_CASE( "Group inheritance covers grandchildren", "[groups]" )
{
	AppFixture fx;
	Command g, p, c;
	g.mName = "root";
	g.mGroups = { "staff" };
	fx.app.commander().add( g );

	p.mName = "mid";
	p.mParentCommand = "root";
	fx.app.commander().add( p );

	c.mName = "leaf";
	c.mParentCommand = "root.mid";
	fx.app.commander().add( c );

	fx.app.commander().configure( {} );

	auto leaf = fx.app.commander().find( "root.mid.leaf" );
	REQUIRE( leaf );
	REQUIRE( leaf->mGroups == std::vector<std::string>{ "staff" } );
}

TEST_CASE( "Invocation of gated command is refused without callback", "[groups]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "secret";
	cmd.mGroups = { "staff" };
	cmd.mNbParams = 0;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "secret" } );

	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE_FALSE( called );
}

TEST_CASE( "Invocation of gated command runs when group enabled", "[groups]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "secret";
	cmd.mGroups = { "staff" };
	cmd.mNbParams = 0;
	fx.app.commander().add( cmd );

	fx.app.commander().setEnabledGroups( { "staff" } );
	fx.app.commander().configure( { "secret" } );

	std::string seen;
	fx.app.commander().run( [&]( const std::string & c ){ seen = c; } );
	REQUIRE( seen == "secret" );
}

TEST_CASE( "requestedCommand reflects parsed dotted path", "[groups]" )
{
	AppFixture fx;
	Command parent;
	parent.mName = "git";
	fx.app.commander().add( parent );

	Command sub;
	sub.mName = "commit";
	sub.mParentCommand = "git";
	fx.app.commander().add( sub );

	fx.app.commander().configure( { "git", "commit", "msg" } );

	REQUIRE( fx.app.commander().requestedCommand() == "git.commit" );
}

TEST_CASE( "App::helpFooter returns empty when no callback installed", "[footer]" )
{
	AppFixture fx;
	REQUIRE( fx.app.helpFooter( {} ).empty() );
	REQUIRE( fx.app.helpFooter( "anything" ).empty() );
}

TEST_CASE( "App::helpFooter calls the installed callback", "[footer]" )
{
	AppFixture fx;
	std::string seenCmd = "<unset>";
	int calls = 0;
	fx.app.setHelpFooter( [&]( const App &, std::string_view command ){
		seenCmd = std::string( command );
		++calls;
		return std::string( "the-footer" );
	});

	REQUIRE( fx.app.helpFooter( {} ) == "the-footer" );
	REQUIRE( seenCmd.empty() );
	REQUIRE( calls == 1 );

	REQUIRE( fx.app.helpFooter( "git.commit" ) == "the-footer" );
	REQUIRE( seenCmd == "git.commit" );
	REQUIRE( calls == 2 );
}

TEST_CASE( "App::helpFooter passes through empty return from callback", "[footer]" )
{
	AppFixture fx;
	int calls = 0;
	fx.app.setHelpFooter( [&]( const App &, std::string_view ){
		++calls;
		return std::string{};
	});
	REQUIRE( fx.app.helpFooter( {} ).empty() );
	REQUIRE( calls == 1 );
}

TEST_CASE( "Run callback is not invoked for gated command", "[groups]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "secret";
	cmd.mGroups = { "staff" };
	cmd.mDescription = "do not leak this";
	cmd.mNbParams = 0;
	fx.app.commander().add( cmd );
	fx.app.config().addDefaults();

	fx.app.commander().configure( { "secret" } );
	fx.app.config().registerUse( "help" );

	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE_FALSE( called );
}

TEST_CASE( "YAML group: scalar parses into mGroups", "[groups]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: yaml-test\n"
		"commands:\n"
		"  - command: secret\n"
		"    description: gated\n"
		"    group: staff\n"
		"  - command: public\n"
		"    description: open\n"
	);

	auto secret = fx.app.commander().find( "secret" );
	REQUIRE( secret );
	REQUIRE( secret->mGroups == std::vector<std::string>{ "staff" } );

	auto pub = fx.app.commander().find( "public" );
	REQUIRE( pub );
	REQUIRE( pub->mGroups.empty() );
}

TEST_CASE( "YAML group: sequence parses into mGroups", "[groups]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: yaml-test\n"
		"commands:\n"
		"  - command: multi\n"
		"    description: gated\n"
		"    group:\n"
		"      - alpha\n"
		"      - beta\n"
	);

	auto multi = fx.app.commander().find( "multi" );
	REQUIRE( multi );
	REQUIRE( multi->mGroups.size() == 2 );
	REQUIRE( multi->mGroups[0] == "alpha" );
	REQUIRE( multi->mGroups[1] == "beta" );
}
