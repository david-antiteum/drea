#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Commander.h>
#include <drea/core/Command.h>

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

TEST_CASE( "Commander::add stores commands", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "run";
	cmd.mDescription = "run something";
	fx.app.commander().add( cmd );

	REQUIRE_FALSE( fx.app.commander().empty() );
	auto found = fx.app.commander().find( "run" );
	REQUIRE( found );
	REQUIRE( found->mName == "run" );
}

TEST_CASE( "Commander::find returns null for unknown", "[commander]" )
{
	AppFixture fx;
	REQUIRE_FALSE( fx.app.commander().find( "ghost" ) );
}

TEST_CASE( "Commander::add batch returns same number of objects", "[commander]" )
{
	AppFixture fx;
	Command a, b;
	a.mName = "alpha";
	b.mName = "beta";
	auto res = fx.app.commander().add( { a, b } );
	REQUIRE( res.size() == 2 );
	REQUIRE( fx.app.commander().find( "alpha" ) );
	REQUIRE( fx.app.commander().find( "beta" ) );
}

TEST_CASE( "Commander::configure extracts arguments after command", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "echo";
	cmd.mParamName = "text";
	cmd.mNbParams = 1;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "echo", "hello" } );
	auto args = fx.app.commander().arguments();
	REQUIRE( args.size() == 1 );
	REQUIRE( args.at( 0 ) == "hello" );
}

TEST_CASE( "Commander::configure finds subcommand and extracts remaining args", "[commander]" )
{
	AppFixture fx;
	Command parent;
	parent.mName = "git";
	fx.app.commander().add( parent );

	Command sub;
	sub.mName = "commit";
	sub.mParentCommand = "git";
	sub.mParamName = "msg";
	sub.mNbParams = 1;
	fx.app.commander().add( sub );

	fx.app.commander().configure( { "git", "commit", "fix-bug" } );
	auto args = fx.app.commander().arguments();
	REQUIRE( args.size() == 1 );
	REQUIRE( args.at( 0 ) == "fix-bug" );
}

TEST_CASE( "Commander::run auto-validates param count and skips callback on mismatch", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "copy";
	cmd.mParamName = "src dst";
	cmd.mNbParams = 2;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "copy", "only-one" } );

	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE_FALSE( called );
}

TEST_CASE( "Commander::run calls callback when param count matches", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "copy";
	cmd.mParamName = "src dst";
	cmd.mNbParams = 2;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "copy", "a", "b" } );

	std::string seen;
	fx.app.commander().run( [&]( const std::string & c ){ seen = c; } );
	REQUIRE( seen == "copy" );
}

TEST_CASE( "Commander::run allows unlimited params", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "sum";
	cmd.mParamName = "values";
	cmd.mNbParams = Command::mUnlimitedParams;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "sum", "1", "2", "3", "4", "5" } );

	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE( called );
}

TEST_CASE( "Commander::run passes callback with zero-param command", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "status";
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "status" } );

	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE( called );
}

TEST_CASE( "Commander::run accepts zero args when min-params is 0 and max is 1", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "status";
	cmd.mParamName = "workflow-id";
	cmd.mNbParams = 1;
	cmd.mMinParams = 0;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "status" } );

	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE( called );
}

TEST_CASE( "Commander::run accepts one arg when min-params is 0 and max is 1", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "status";
	cmd.mParamName = "workflow-id";
	cmd.mNbParams = 1;
	cmd.mMinParams = 0;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "status", "42" } );

	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE( called );
}

TEST_CASE( "Commander::run rejects over-max args with min-params set", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "status";
	cmd.mParamName = "workflow-id";
	cmd.mNbParams = 1;
	cmd.mMinParams = 0;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "status", "42", "extra" } );

	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE_FALSE( called );
}
