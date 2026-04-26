#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Commander.h>
#include <drea/core/Command.h>
#include <drea/core/Config.h>
#include <drea/core/Option.h>

#include <iostream>
#include <sstream>

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

// Capture std::cout during a callable's execution.
namespace {
struct CoutCapture {
	std::ostringstream			buffer;
	std::streambuf *			oldBuf;
	CoutCapture() : oldBuf( std::cout.rdbuf( buffer.rdbuf() ) ) {}
	~CoutCapture() { std::cout.rdbuf( oldBuf ); }
	std::string str() const { return buffer.str(); }
};

struct BuiltinFixture {
	char  argv0[16] = "myapp";
	char* argv[1]   = { argv0 };
	App   app;
	BuiltinFixture() : app( 1, argv ) {
		app.setName( "myapp" );
		app.setVersion( "1.2.3" );
		app.setDescription( "Test application" );

		Command hello;
		hello.mName = "hello";
		hello.mDescription = "print a greeting";
		hello.mLocalParameters = { "loud" };
		app.commander().add( hello );

		drea::core::Option loud;
		loud.mName = "loud";
		loud.mDescription = "shout it";
		loud.mType = typeid( bool );
		loud.mNbParams = 0;
		app.config().add( loud );

		app.config().addDefaults();
		app.commander().addDefaults();
	}
};
}

TEST_CASE( "Commander::addDefaults registers completion and man builtins", "[commander][builtins]" )
{
	BuiltinFixture fx;
	REQUIRE( fx.app.commander().find( "completion" ) );
	REQUIRE( fx.app.commander().find( "man" ) );
}

TEST_CASE( "completion bash produces a bash script", "[commander][builtins]" )
{
	BuiltinFixture fx;
	fx.app.commander().configure( { "completion", "bash" } );

	CoutCapture cap;
	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE_FALSE( called );

	const std::string out = cap.str();
	REQUIRE( out.find( "#!/usr/bin/env bash" ) != std::string::npos );
	REQUIRE( out.find( "_myapp()" ) != std::string::npos );
	REQUIRE( out.find( "complete -F _myapp myapp" ) != std::string::npos );
	REQUIRE( out.find( "hello" ) != std::string::npos );
	REQUIRE( out.find( "--loud" ) != std::string::npos );
}

TEST_CASE( "completion zsh produces a compdef script", "[commander][builtins]" )
{
	BuiltinFixture fx;
	fx.app.commander().configure( { "completion", "zsh" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	const std::string out = cap.str();
	REQUIRE( out.find( "#compdef myapp" ) != std::string::npos );
	REQUIRE( out.find( "_myapp_opts_for" ) != std::string::npos );
	REQUIRE( out.find( "'hello:print a greeting'" ) != std::string::npos );
}

TEST_CASE( "completion fish produces fish completions", "[commander][builtins]" )
{
	BuiltinFixture fx;
	fx.app.commander().configure( { "completion", "fish" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	const std::string out = cap.str();
	REQUIRE( out.find( "complete -c myapp" ) != std::string::npos );
	REQUIRE( out.find( "-a 'hello'" ) != std::string::npos );
	REQUIRE( out.find( "-l loud" ) != std::string::npos );
}

TEST_CASE( "completion with unsupported shell does not print a script", "[commander][builtins]" )
{
	BuiltinFixture fx;
	fx.app.commander().configure( { "completion", "powershell" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	REQUIRE( cap.str().find( "#!/usr/bin/env bash" ) == std::string::npos );
	REQUIRE( cap.str().find( "#compdef" ) == std::string::npos );
}

TEST_CASE( "man produces a roff man page", "[commander][builtins]" )
{
	BuiltinFixture fx;
	fx.app.commander().configure( { "man" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	const std::string out = cap.str();
	REQUIRE( out.find( ".TH MYAPP 1" ) != std::string::npos );
	REQUIRE( out.find( ".SH NAME" ) != std::string::npos );
	REQUIRE( out.find( ".SH SYNOPSIS" ) != std::string::npos );
	REQUIRE( out.find( ".SH COMMANDS" ) != std::string::npos );
	REQUIRE( out.find( "myapp 1.2.3" ) != std::string::npos );
	REQUIRE( out.find( "hello" ) != std::string::npos );
}

TEST_CASE( "man <command> produces a per-command man page", "[commander][builtins]" )
{
	BuiltinFixture fx;
	fx.app.commander().configure( { "man", "hello" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	const std::string out = cap.str();
	REQUIRE( out.find( ".TH MYAPP\\-HELLO 1" ) != std::string::npos );
	REQUIRE( out.find( "print a greeting" ) != std::string::npos );
	REQUIRE( out.find( ".BR myapp (1)" ) != std::string::npos );
}

TEST_CASE( "user command named completion takes precedence over builtin", "[commander][builtins]" )
{
	char argv0[] = "myapp";
	char* argv[] = { argv0 };
	App app( 1, argv );
	app.setName( "myapp" );

	Command userCompletion;
	userCompletion.mName = "completion";
	userCompletion.mDescription = "user-defined";
	userCompletion.mNbParams = 0;
	app.commander().add( userCompletion );

	app.config().addDefaults();
	app.commander().addDefaults();

	app.commander().configure( { "completion" } );

	CoutCapture cap;
	bool called = false;
	app.commander().run( [&]( const std::string & cmd ){ if( cmd == "completion" ) called = true; } );

	REQUIRE( called );
	REQUIRE( cap.str().find( "#!/usr/bin/env bash" ) == std::string::npos );
}

TEST_CASE( "Commander::remove erases a command", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "drop-me";
	fx.app.commander().add( cmd );

	REQUIRE( fx.app.commander().find( "drop-me" ) );

	fx.app.commander().remove( "drop-me" );

	REQUIRE_FALSE( fx.app.commander().find( "drop-me" ) );
}

TEST_CASE( "Commander::remove on a parent erases descendants", "[commander]" )
{
	AppFixture fx;
	Command parent;
	parent.mName = "container";
	fx.app.commander().add( parent );

	Command sub;
	sub.mName = "ls";
	sub.mParentCommand = "container";
	fx.app.commander().add( sub );

	Command grand;
	grand.mName = "all";
	grand.mParentCommand = "container.ls";
	fx.app.commander().add( grand );

	fx.app.commander().remove( "container" );

	REQUIRE_FALSE( fx.app.commander().find( "container" ) );
	REQUIRE_FALSE( fx.app.commander().find( "container.ls" ) );
	REQUIRE_FALSE( fx.app.commander().find( "container.ls.all" ) );
}

TEST_CASE( "Commander::remove can drop the man builtin", "[commander][builtins]" )
{
	BuiltinFixture fx;
	REQUIRE( fx.app.commander().find( "man" ) );

	fx.app.commander().remove( "man" );

	REQUIRE_FALSE( fx.app.commander().find( "man" ) );
}

TEST_CASE( "Commander::remove on unknown name is a no-op", "[commander]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "kept";
	fx.app.commander().add( cmd );

	fx.app.commander().remove( "missing" );

	REQUIRE( fx.app.commander().find( "kept" ) );
}
