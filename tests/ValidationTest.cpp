#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Command.h>
#include <drea/core/Commander.h>
#include <drea/core/Config.h>
#include <drea/core/Option.h>

#include <string>

using drea::core::App;
using drea::core::Option;

namespace {

struct AppFixture {
	char  argv0[16] = "drea-test";
	char* argv[1]   = { argv0 };
	App   app;
	AppFixture() : app( 1, argv ) {}
};

}

TEST_CASE( "validate flags a missing required option", "[validate]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "pool-id";
	opt.mParamName = "id";
	opt.mType = typeid( std::string );
	opt.mRequired = true;
	fx.app.config().add( opt );

	fx.app.config().configure( {} );

	const auto errors = fx.app.config().validate();
	REQUIRE( errors.size() == 1 );
	REQUIRE( errors.front().find( "pool-id" ) != std::string::npos );
}

TEST_CASE( "validate passes when the required option is set", "[validate]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "pool-id";
	opt.mParamName = "id";
	opt.mType = typeid( std::string );
	opt.mRequired = true;
	fx.app.config().add( opt );

	fx.app.config().configure( { "--pool-id", "eu-1" } );

	REQUIRE( fx.app.config().validate().empty() );
}

TEST_CASE( "required option with a default never fails", "[validate]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "port";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	opt.mRequired = true;
	opt.mValues = { 8080 };
	fx.app.config().add( opt );

	fx.app.config().configure( {} );

	REQUIRE( fx.app.config().validate().empty() );
}

TEST_CASE( "validate enforces min and max on numeric options", "[validate]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "port";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	opt.mMin = 1;
	opt.mMax = 65535;
	fx.app.config().add( opt );

	SECTION( "value in range" ){
		fx.app.config().configure( { "--port", "8080" } );
		REQUIRE( fx.app.config().validate().empty() );
	}
	SECTION( "value below min" ){
		fx.app.config().configure( { "--port", "0" } );
		const auto errors = fx.app.config().validate();
		REQUIRE( errors.size() == 1 );
		REQUIRE( errors.front().find( "below the minimum" ) != std::string::npos );
	}
	SECTION( "value above max" ){
		fx.app.config().configure( { "--port=70000" } );
		const auto errors = fx.app.config().validate();
		REQUIRE( errors.size() == 1 );
		REQUIRE( errors.front().find( "above the maximum" ) != std::string::npos );
	}
}

TEST_CASE( "min and max work on double options", "[validate]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "ratio";
	opt.mParamName = "x";
	opt.mType = typeid( double );
	opt.mMin = 0.0;
	opt.mMax = 1.0;
	fx.app.config().add( opt );

	fx.app.config().configure( { "--ratio", "1.5" } );

	REQUIRE( fx.app.config().validate().size() == 1 );
}

TEST_CASE( "unset optional numeric option does not trigger range errors", "[validate]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "port";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	opt.mMin = 1;
	fx.app.config().add( opt );

	fx.app.config().configure( {} );

	REQUIRE( fx.app.config().validate().empty() );
}

TEST_CASE( "parse reads required/min/max from yml definitions", "[validate]" )
{
	AppFixture fx;
	fx.app.parse( R"(
app: drea-test
options:
  - option: threads
    description: worker threads
    params-names: n
    type: int
    min: 1
    max: 64
    value: 4
  - option: token
    description: api token
    params-names: t
    type: string
    required: true
    value: abc
)" );

	auto threads = fx.app.config().find( "threads" );
	REQUIRE( threads );
	REQUIRE( threads->mMin.has_value() );
	REQUIRE( *threads->mMin == 1.0 );
	REQUIRE( threads->mMax.has_value() );
	REQUIRE( *threads->mMax == 64.0 );
	auto token = fx.app.config().find( "token" );
	REQUIRE( token );
	REQUIRE( token->mRequired );
	REQUIRE( fx.app.config().validate().empty() );
}

TEST_CASE( "config sources are tracked per option", "[config-source]" )
{
	AppFixture fx;
	Option flag;
	flag.mName = "level";
	flag.mParamName = "n";
	flag.mType = typeid( int );
	fx.app.config().add( flag );
	Option def;
	def.mName = "size";
	def.mParamName = "n";
	def.mType = typeid( int );
	def.mValues = { 7 };
	fx.app.config().add( def );

	fx.app.config().configure( { "--level", "3" } );

	REQUIRE( fx.app.config().source( "level" ) == "flag" );
	REQUIRE( fx.app.config().source( "size" ) == "default" );

	fx.app.config().set( "size", "9" );
	REQUIRE( fx.app.config().source( "size" ) == "code" );
}

TEST_CASE( "validate flags a value outside choices", "[validate]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "color";
	opt.mParamName = "mode";
	opt.mType = typeid( std::string );
	opt.mChoices = { "auto", "always", "never" };
	fx.app.config().add( opt );

	fx.app.config().configure( { "--color", "sometimes" } );

	const auto errors = fx.app.config().validate();
	REQUIRE( errors.size() == 1 );
	REQUIRE( errors.front().find( "sometimes" ) != std::string::npos );
	REQUIRE( errors.front().find( "auto, always, never" ) != std::string::npos );
}

TEST_CASE( "validate passes a value inside choices", "[validate]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "color";
	opt.mParamName = "mode";
	opt.mType = typeid( std::string );
	opt.mChoices = { "auto", "always", "never" };
	fx.app.config().add( opt );

	fx.app.config().configure( { "--color", "never" } );

	REQUIRE( fx.app.config().validate().empty() );
}

TEST_CASE( "validate flags a command referencing an unknown option", "[validate]" )
{
	AppFixture fx;
	drea::core::Command cmd;
	cmd.mName = "start";
	cmd.mLocalParameters = { "does-not-exist" };
	fx.app.commander().add( cmd );

	fx.app.config().configure( {} );

	const auto errors = fx.app.config().validate();
	REQUIRE( errors.size() == 1 );
	REQUIRE( errors.front().find( "start" ) != std::string::npos );
	REQUIRE( errors.front().find( "does-not-exist" ) != std::string::npos );
}
