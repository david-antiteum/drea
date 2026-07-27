#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Commander.h>
#include <drea/core/Command.h>
#include <drea/core/Config.h>
#include <drea/core/Option.h>

#include "integrations/bash/bash_completion.h"
#include "integrations/fish/fish_completion.h"
#include "integrations/help/help.h"
#include "integrations/man/man.h"
#include "integrations/zsh/zsh_completion.h"

#include <algorithm>
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

TEST_CASE( "Commander::addDefaults registers the completion, man and describe builtins", "[commander][builtins]" )
{
	BuiltinFixture fx;
	REQUIRE( fx.app.commander().find( "completion" ) );
	REQUIRE( fx.app.commander().find( "man" ) );
	REQUIRE( fx.app.commander().find( "describe" ) );
	// describe is a command, not an option
	REQUIRE_FALSE( fx.app.config().find( "describe" ) );
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

TEST_CASE( "Commander::addDefaults marks its commands as predefined", "[commander]" )
{
	BuiltinFixture fx;

	REQUIRE( fx.app.commander().find( "completion" )->mPredefined );
	REQUIRE( fx.app.commander().find( "man" )->mPredefined );
	REQUIRE_FALSE( fx.app.commander().find( "hello" )->mPredefined );
}

TEST_CASE( "describe prints the app description as JSON", "[commander][describe]" )
{
	BuiltinFixture fx;
	fx.app.commander().configure( { "describe" } );

	CoutCapture cap;
	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE_FALSE( called );

	const std::string out = cap.str();
	REQUIRE( out.find( "\"schema\": \"drea-describe/1\"" ) != std::string::npos );
	REQUIRE( out.find( "\"app\": \"myapp\"" ) != std::string::npos );
	REQUIRE( out.find( "\"version\": \"1.2.3\"" ) != std::string::npos );
	REQUIRE( out.find( "\"name\": \"hello\"" ) != std::string::npos );
	REQUIRE( out.find( "\"local-options\": [\"loud\"]" ) != std::string::npos );
	REQUIRE( out.find( "\"name\": \"describe\"" ) != std::string::npos );
	REQUIRE( out.find( "\"type\": \"bool\"" ) != std::string::npos );
	REQUIRE( out.find( "\"usage\": \"myapp COMMAND [SUBCOMMAND ...] [PARAMS] [OPTIONS]\"" ) != std::string::npos );
	REQUIRE( out.find( "\"conventions\": {" ) != std::string::npos );
	REQUIRE( out.find( "--no-name disables" ) != std::string::npos );
	REQUIRE( out.find( "\"predefined\":" ) == std::string::npos );
	// crude structural check: balanced braces and brackets
	REQUIRE( std::count( out.begin(), out.end(), '{' ) == std::count( out.begin(), out.end(), '}' ) );
	REQUIRE( std::count( out.begin(), out.end(), '[' ) == std::count( out.begin(), out.end(), ']' ) );
}

TEST_CASE( "describe includes limits, defaults and env var names", "[commander][describe]" )
{
	BuiltinFixture fx;

	drea::core::Option threshold;
	threshold.mName = "threshold";
	threshold.mParamName = "value";
	threshold.mType = typeid( double );
	threshold.mValues = { 0.5 };
	threshold.mMin = 0.0;
	threshold.mMax = 1.0;
	fx.app.config().add( threshold );

	drea::core::Option secret;
	secret.mName = "api-key";
	secret.mParamName = "key";
	secret.mType = typeid( std::string );
	secret.mValues = { std::string( "s3cr3t" ) };
	secret.mSensitive = true;
	fx.app.config().add( secret );

	fx.app.config().setEnvPrefix( "MYAPP" );
	fx.app.commander().configure( { "describe" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	const std::string out = cap.str();
	REQUIRE( out.find( "\"min\": 0" ) != std::string::npos );
	REQUIRE( out.find( "\"max\": 1" ) != std::string::npos );
	REQUIRE( out.find( "\"default\": [0.5]" ) != std::string::npos );
	REQUIRE( out.find( "\"env\": \"MYAPP_api_key\"" ) != std::string::npos );
	REQUIRE( out.find( "\"default\": \"[redacted]\"" ) != std::string::npos );
	REQUIRE( out.find( "s3cr3t" ) == std::string::npos );
}

TEST_CASE( "describe omits hidden commands", "[commander][describe]" )
{
	BuiltinFixture fx;

	Command ghost;
	ghost.mName = "ghost";
	ghost.mDescription = "internal";
	ghost.mHidden = true;
	fx.app.commander().add( ghost );

	fx.app.commander().configure( { "describe" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	REQUIRE( cap.str().find( "\"ghost\"" ) == std::string::npos );
}

TEST_CASE( "describe nests subcommands as JSON objects", "[commander][describe]" )
{
	BuiltinFixture fx;

	Command box;
	box.mName = "box";
	box.mDescription = "box things";
	fx.app.commander().add( box );

	Command open;
	open.mName = "open";
	open.mParentCommand = "box";
	open.mDescription = "open the box";
	fx.app.commander().add( open );

	fx.app.commander().configure( { "describe" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	const std::string out = cap.str();
	const auto boxPos = out.find( "\"name\": \"box\"" );
	const auto openPos = out.find( "\"name\": \"open\"" );
	REQUIRE( boxPos != std::string::npos );
	REQUIRE( openPos != std::string::npos );
	// the subcommand object comes nested after its parent, inside a
	// "commands" array, and the flat-list linking fields are gone
	REQUIRE( openPos > boxPos );
	const auto nestedArrayPos = out.find( "\"commands\": [", boxPos );
	REQUIRE( nestedArrayPos != std::string::npos );
	REQUIRE( nestedArrayPos < openPos );
	REQUIRE( out.find( "\"full-name\":" ) == std::string::npos );
	REQUIRE( out.find( "\"parent\":" ) == std::string::npos );
	REQUIRE( out.find( "\"subcommands\":" ) == std::string::npos );
}

TEST_CASE( "describe lists choices and machine readable value sets", "[commander][describe]" )
{
	BuiltinFixture fx;

	drea::core::Option color;
	color.mName = "color";
	color.mParamName = "mode";
	color.mType = typeid( std::string );
	color.mChoices = { "auto", "always", "never" };
	fx.app.config().add( color );

	fx.app.commander().configure( { "describe" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	const std::string out = cap.str();
	REQUIRE( out.find( "\"choices\": [\"auto\", \"always\", \"never\"]" ) != std::string::npos );
	REQUIRE( out.find( "\"option-types\": [\"bool\", \"int\", \"double\", \"string\"]" ) != std::string::npos );
	REQUIRE( out.find( "\"option-scopes\": [\"both\", \"command-line\", \"config-file\", \"none\"]" ) != std::string::npos );
	REQUIRE( out.find( "\"env-derivation\":" ) != std::string::npos );
}

TEST_CASE( "describe lists the groups of a visible gated command", "[commander][describe]" )
{
	BuiltinFixture fx;

	Command gated;
	gated.mName = "beta-tools";
	gated.mDescription = "beta features";
	gated.mGroups = { "beta" };
	fx.app.commander().add( gated );
	fx.app.commander().setEnabledGroups( { "beta" } );

	fx.app.commander().configure( { "describe" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	const std::string out = cap.str();
	REQUIRE( out.find( "\"name\": \"beta-tools\"" ) != std::string::npos );
	REQUIRE( out.find( "\"groups\": [\"beta\"]" ) != std::string::npos );
}

TEST_CASE( "describe omits env for command-line scoped options and marks deprecated", "[commander][describe]" )
{
	BuiltinFixture fx;

	drea::core::Option cliOnly;
	cliOnly.mName = "burst";
	cliOnly.mDescription = "one shot tuning";
	cliOnly.mParamName = "n";
	cliOnly.mType = typeid( int );
	cliOnly.mScope = drea::core::Option::Scope::Line;
	cliOnly.mDeprecated = true;
	fx.app.config().add( cliOnly );

	fx.app.config().setEnvPrefix( "MYAPP" );
	fx.app.commander().configure( { "describe" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	const std::string out = cap.str();
	REQUIRE( out.find( "\"env\": \"MYAPP_burst\"" ) == std::string::npos );
	REQUIRE( out.find( "\"env\": \"MYAPP_loud\"" ) != std::string::npos );
	const auto burstPos = out.find( "\"name\": \"burst\"" );
	REQUIRE( burstPos != std::string::npos );
	REQUIRE( out.find( "\"deprecated\": true", burstPos ) != std::string::npos );
}

TEST_CASE( "describe lists command examples", "[commander][describe]" )
{
	BuiltinFixture fx;

	Command copy;
	copy.mName = "copy";
	copy.mDescription = "copy a file";
	copy.mParamName = "src dst";
	copy.mNbParams = 2;
	copy.mExamples = { "myapp copy in.txt out.txt" };
	fx.app.commander().add( copy );

	fx.app.commander().configure( { "describe" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	REQUIRE( cap.str().find( "\"examples\": [\"myapp copy in.txt out.txt\"]" ) != std::string::npos );
}

TEST_CASE( "describe reports the declared default, not the resolved value", "[commander][describe]" )
{
	BuiltinFixture fx;

	drea::core::Option threshold;
	threshold.mName = "threshold";
	threshold.mParamName = "value";
	threshold.mType = typeid( double );
	threshold.mValues = { 0.5 };
	fx.app.config().add( threshold );

	fx.app.config().configure( { "--threshold", "0.9" } );
	fx.app.commander().configure( { "describe" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );

	const std::string out = cap.str();
	REQUIRE( out.find( "\"default\": [0.5]" ) != std::string::npos );
	REQUIRE( out.find( "0.9" ) == std::string::npos );
}

TEST_CASE( "completion scripts offer the values of options with choices", "[commander][completion]" )
{
	BuiltinFixture fx;

	drea::core::Option tier;
	tier.mName = "tier";
	tier.mParamName = "tier";
	tier.mType = typeid( std::string );
	tier.mChoices = { "none", "readonly", "operator" };
	fx.app.config().add( tier );
	// attach to a command so zsh/fish emit it
	fx.app.commander().find( "hello" )->mGlobalParameters = { "tier" };

	fx.app.config().configure( {} );
	fx.app.commander().configure( {} );

	std::ostringstream bash, zsh, fish;
	drea::core::integrations::Bash::generateAutoCompletion( fx.app, bash );
	drea::core::integrations::Zsh::generateAutoCompletion( fx.app, zsh );
	drea::core::integrations::Fish::generateAutoCompletion( fx.app, fish );

	REQUIRE( bash.str().find( "--tier)" ) != std::string::npos );
	REQUIRE( bash.str().find( "none readonly operator" ) != std::string::npos );
	REQUIRE( zsh.str().find( "(none readonly operator)" ) != std::string::npos );
	REQUIRE( fish.str().find( "-x -a 'none readonly operator'" ) != std::string::npos );
}

TEST_CASE( "man pages render choices, defaults and required", "[commander][man]" )
{
	BuiltinFixture fx;

	drea::core::Option tier;
	tier.mName = "tier";
	tier.mParamName = "tier";
	tier.mType = typeid( std::string );
	tier.mChoices = { "none", "readonly", "operator" };
	tier.mValues = { std::string( "none" ) };
	tier.mRequired = true;
	fx.app.config().add( tier );

	fx.app.config().configure( { "--tier", "operator" } );
	fx.app.commander().configure( {} );

	std::ostringstream man;
	drea::core::integrations::Man::generateManPage( fx.app, man );

	const std::string page = man.str();
	REQUIRE( page.find( "One of: none, readonly, operator" ) != std::string::npos );
	// the declared default, not the value the flag resolved to
	REQUIRE( page.find( "Default none" ) != std::string::npos );
	REQUIRE( page.find( "Required" ) != std::string::npos );
}

TEST_CASE( "param-choices gate the positional argument", "[commander][param-choices]" )
{
	BuiltinFixture fx;

	Command deploy;
	deploy.mName = "deploy";
	deploy.mParamName = "stage";
	deploy.mNbParams = 1;
	deploy.mParamChoices = { "dev", "prod" };
	fx.app.commander().add( deploy );

	SECTION( "a declared value runs the command" ){
		fx.app.config().configure( {} );
		fx.app.commander().configure( { "deploy", "prod" } );
		bool called = false;
		fx.app.commander().run( [&]( const std::string & cmd ){ called = ( cmd == "deploy" ); } );
		REQUIRE( called );
	}
	SECTION( "an unknown value is rejected before dispatch" ){
		fx.app.config().configure( {} );
		fx.app.commander().configure( { "deploy", "prd" } );
		bool called = false;
		fx.app.commander().run( [&]( const std::string & ){ called = true; } );
		REQUIRE_FALSE( called );
	}
}

TEST_CASE( "the completion builtin declares its shells as param-choices", "[commander][param-choices]" )
{
	BuiltinFixture fx;

	auto completion = fx.app.commander().find( "completion" );
	REQUIRE( completion );
	REQUIRE( completion->mParamChoices == std::vector<std::string>{ "bash", "zsh", "fish" } );

	// scripts and describe pick them up
	fx.app.commander().configure( { "describe" } );

	std::ostringstream bash, zsh, fish;
	drea::core::integrations::Bash::generateAutoCompletion( fx.app, bash );
	drea::core::integrations::Zsh::generateAutoCompletion( fx.app, zsh );
	drea::core::integrations::Fish::generateAutoCompletion( fx.app, fish );
	REQUIRE( bash.str().find( " bash zsh fish" ) != std::string::npos );
	REQUIRE( zsh.str().find( "'1: :(bash zsh fish)'" ) != std::string::npos );
	REQUIRE( fish.str().find( "-a 'bash zsh fish'" ) != std::string::npos );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );
	REQUIRE( cap.str().find( "\"param-choices\": [\"bash\", \"zsh\", \"fish\"]" ) != std::string::npos );
}

TEST_CASE( "an app with root params takes arguments with no command", "[commander][root]" )
{
	AppFixture fx;
	Command root;
	root.mParamName = "name";
	root.mNbParams = Command::mUnlimitedParams;
	fx.app.commander().setRoot( root );

	fx.app.commander().configure( { "Ada", "Alan" } );

	REQUIRE( fx.app.commander().requestedCommand().empty() );
	REQUIRE_FALSE( fx.app.commander().invalidCommand() );
	REQUIRE( fx.app.commander().arguments() == std::vector<std::string>{ "Ada", "Alan" } );

	std::string seen = "unset";
	fx.app.commander().run( [&]( const std::string & c ){ seen = c; } );
	REQUIRE( seen.empty() );
}

TEST_CASE( "a command wins over root params", "[commander][root]" )
{
	AppFixture fx;
	Command root;
	root.mParamName = "name";
	root.mNbParams = Command::mUnlimitedParams;
	fx.app.commander().setRoot( root );

	Command cmd;
	cmd.mName = "status";
	cmd.mNbParams = 0;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "status" } );

	REQUIRE( fx.app.commander().requestedCommand() == "status" );
	REQUIRE( fx.app.commander().arguments().empty() );
}

TEST_CASE( "a leading -- forces the arguments to be root params", "[commander][root]" )
{
	AppFixture fx;
	Command root;
	root.mParamName = "name";
	root.mNbParams = Command::mUnlimitedParams;
	fx.app.commander().setRoot( root );

	Command cmd;
	cmd.mName = "status";
	cmd.mNbParams = 0;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "--", "status" } );

	REQUIRE( fx.app.commander().requestedCommand().empty() );
	REQUIRE_FALSE( fx.app.commander().invalidCommand() );
	REQUIRE( fx.app.commander().arguments() == std::vector<std::string>{ "status" } );
}

TEST_CASE( "root params are counted like the params of a command", "[commander][root]" )
{
	AppFixture fx;
	Command root;
	root.mParamName = "src dst";
	root.mNbParams = 2;
	fx.app.commander().setRoot( root );

	fx.app.commander().configure( { "only-one" } );

	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE_FALSE( called );
}

TEST_CASE( "root param-choices are checked", "[commander][root]" )
{
	AppFixture fx;
	Command root;
	root.mParamName = "stage";
	root.mNbParams = 1;
	root.mParamChoices = { "dev", "prod" };
	fx.app.commander().setRoot( root );

	fx.app.commander().configure( { "staging" } );

	bool called = false;
	fx.app.commander().run( [&]( const std::string & ){ called = true; } );
	REQUIRE_FALSE( called );
}

TEST_CASE( "without root params an argument that is not a command is invalid", "[commander][root]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "status";
	cmd.mNbParams = 0;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "stauts" } );

	// run() quits with ExitCode::UsageError here, so the flag is what a test
	// can check: the callback is never reached
	REQUIRE( fx.app.commander().invalidCommand() );
	REQUIRE( fx.app.commander().requestedCommand().empty() );
}

TEST_CASE( "-- without root params is invalid", "[commander][root]" )
{
	AppFixture fx;
	Command cmd;
	cmd.mName = "status";
	cmd.mNbParams = 0;
	fx.app.commander().add( cmd );

	fx.app.commander().configure( { "--", "status" } );

	REQUIRE( fx.app.commander().invalidCommand() );
}

TEST_CASE( "parse reads the root block from yml definitions", "[commander][root]" )
{
	AppFixture fx;
	fx.app.parse( R"(
app: drea-test
root:
  params-names: name
  params: unlimited
  min-params: 1
  description: greets every name
  examples:
    - drea-test Ada
)" );

	auto root = fx.app.commander().root();
	REQUIRE( root );
	REQUIRE( root->mName.empty() );
	REQUIRE( root->mParamName == "name" );
	REQUIRE( root->mNbParams == Command::mUnlimitedParams );
	REQUIRE( root->mMinParams == 1 );
	REQUIRE( root->mDescription == "greets every name" );
	REQUIRE( root->mExamples == std::vector<std::string>{ "drea-test Ada" } );
	REQUIRE( root->nameOfParamsForHelp() == "<name>..." );
}

TEST_CASE( "hasAppCommands ignores the builtins", "[commander][root]" )
{
	BuiltinFixture fx;
	REQUIRE( fx.app.commander().hasAppCommands() );

	AppFixture bare;
	bare.app.commander().addDefaults();
	REQUIRE_FALSE( bare.app.commander().empty() );
	REQUIRE_FALSE( bare.app.commander().hasAppCommands() );
}

TEST_CASE( "the usage line reflects root params and commands", "[commander][root][help]" )
{
	{
		// root params and commands: both forms are documented
		BuiltinFixture fx;
		Command root;
		root.mParamName = "name";
		root.mNbParams = Command::mUnlimitedParams;
		fx.app.commander().setRoot( root );

		REQUIRE( drea::core::integrations::Help::usageLines( fx.app ) ==
			"usage: myapp [OPTIONS] [name]...\n"
			"       myapp COMMAND [OPTIONS]\n" );
	}
	{
		// commands of its own: a command is required
		BuiltinFixture fx;
		REQUIRE( drea::core::integrations::Help::usageLines( fx.app ) == "usage: myapp COMMAND [OPTIONS]\n" );
	}
	{
		// only the builtins: a command is one option among others
		AppFixture fx;
		fx.app.setName( "bare" );
		fx.app.commander().addDefaults();
		REQUIRE( drea::core::integrations::Help::usageLines( fx.app ) == "usage: bare [COMMAND] [OPTIONS]\n" );
	}
	{
		// neither commands nor root params
		AppFixture fx;
		fx.app.setName( "bare" );
		REQUIRE( drea::core::integrations::Help::usageLines( fx.app ) == "usage: bare [OPTIONS]\n" );
	}
}

TEST_CASE( "describe reports the root params", "[commander][root][describe]" )
{
	BuiltinFixture fx;
	Command root;
	root.mParamName = "name";
	root.mNbParams = Command::mUnlimitedParams;
	root.mMinParams = 1;
	fx.app.commander().setRoot( root );

	fx.app.commander().configure( { "describe" } );

	CoutCapture cap;
	fx.app.commander().run( [&]( const std::string & ){} );
	const std::string out = cap.str();
	REQUIRE( out.find( "\"root\": {" ) != std::string::npos );
	REQUIRE( out.find( "\"params-names\": \"name\"" ) != std::string::npos );
	REQUIRE( out.find( "\"min-params\": 1" ) != std::string::npos );
	REQUIRE( out.find( "\"max-params\": \"unlimited\"" ) != std::string::npos );
}

TEST_CASE( "the man synopsis reflects root params", "[commander][root][man]" )
{
	BuiltinFixture fx;
	Command root;
	root.mParamName = "name";
	root.mNbParams = Command::mUnlimitedParams;
	fx.app.commander().setRoot( root );

	std::ostringstream out;
	drea::core::integrations::Man::generateManPage( fx.app, out );
	REQUIRE( out.str().find( "[\\fIOPTIONS\\fR] \\fIname\\fR..." ) != std::string::npos );
	REQUIRE( out.str().find( "[\\fIOPTIONS\\fR] \\fICOMMAND\\fR [\\fIARGS\\fR...]" ) != std::string::npos );
}

TEST_CASE( "the builtins document what to do with their output", "[commander][builtins]" )
{
	BuiltinFixture fx;

	auto completion = fx.app.commander().find( "completion" );
	REQUIRE( completion );
	REQUIRE( completion->mDescription.find( "myapp" ) != std::string::npos );
	REQUIRE( completion->mExamples.size() > 1 );
	REQUIRE( completion->mExamples.front() == "eval \"$(myapp completion bash)\"" );

	auto man = fx.app.commander().find( "man" );
	REQUIRE( man );
	REQUIRE_FALSE( man->mExamples.empty() );

	// a multi line description stays a one liner in the command list and in the
	// generated completion scripts
	std::ostringstream zsh, fish;
	drea::core::integrations::Zsh::generateAutoCompletion( fx.app, zsh );
	drea::core::integrations::Fish::generateAutoCompletion( fx.app, fish );
	REQUIRE( zsh.str().find( "\n" ) != std::string::npos );
	REQUIRE( zsh.str().find( "'completion:Print a shell completion script for myapp to stdout.'" ) != std::string::npos );
	REQUIRE( fish.str().find( "-d 'Print a shell completion script for myapp to stdout.'" ) != std::string::npos );
}

TEST_CASE( "parse reads param-choices from yml definitions", "[commander][param-choices]" )
{
	AppFixture fx;
	fx.app.parse( R"(
app: drea-test
commands:
  - command: deploy
    description: deploy the app
    params-names: stage
    params: 1
    param-choices: [ dev, staging, prod ]
)" );

	auto cmd = fx.app.commander().find( "deploy" );
	REQUIRE( cmd );
	REQUIRE( cmd->mParamChoices == std::vector<std::string>{ "dev", "staging", "prod" } );
	REQUIRE( fx.app.config().validate().empty() );
}

TEST_CASE( "the man page renders the examples of a command", "[commander][man]" )
{
	BuiltinFixture fx;
	auto hello = fx.app.commander().find( "hello" );
	REQUIRE( hello );
	hello->mExamples = { "myapp hello --loud", "./myapp hello" };

	std::ostringstream out;
	drea::core::integrations::Man::generateManPage( fx.app, "hello", out );
	const std::string page = out.str();

	REQUIRE( page.find( ".SH EXAMPLES" ) != std::string::npos );
	// hyphens escaped so the flag stays copy pasteable
	REQUIRE( page.find( "myapp hello \\-\\-loud" ) != std::string::npos );
	// a leading dot would otherwise be read as a roff request and the line lost
	REQUIRE( page.find( "\\&./myapp hello" ) != std::string::npos );
	// unfilled, so a command line is not reflowed
	REQUIRE( page.find( ".nf" ) != std::string::npos );
}

TEST_CASE( "the man page of an app carries the root description and examples", "[commander][man][root]" )
{
	BuiltinFixture fx;
	Command root;
	root.mParamName = "name";
	root.mNbParams = Command::mUnlimitedParams;
	root.mDescription = "Greets every name given";
	root.mExamples = { "myapp Ada Alan" };
	fx.app.commander().setRoot( root );

	std::ostringstream out;
	drea::core::integrations::Man::generateManPage( fx.app, out );
	const std::string page = out.str();

	REQUIRE( page.find( "Greets every name given" ) != std::string::npos );
	REQUIRE( page.find( ".SH EXAMPLES" ) != std::string::npos );
	REQUIRE( page.find( "myapp Ada Alan" ) != std::string::npos );
	// EXAMPLES belongs before ENVIRONMENT, as man pages conventionally order them
	REQUIRE( page.find( ".SH EXAMPLES" ) < page.find( ".SH ENVIRONMENT" ) );
}

TEST_CASE( "a page without examples has no EXAMPLES section", "[commander][man]" )
{
	BuiltinFixture fx;

	std::ostringstream out;
	drea::core::integrations::Man::generateManPage( fx.app, "hello", out );

	REQUIRE( out.str().find( ".SH EXAMPLES" ) == std::string::npos );
}

TEST_CASE( "the man synopsis of a command has no trailing space", "[commander][man]" )
{
	BuiltinFixture fx;

	std::ostringstream out;
	drea::core::integrations::Man::generateManPage( fx.app, "completion", out );

	for( const auto & line: drea::core::utilities::string::split( out.str(), "\n" ) ){
		REQUIRE( ( line.empty() || line.back() != ' ' ) );
	}
}
