#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Command.h>
#include <drea/core/Commander.h>
#include <drea/core/Config.h>
#include <drea/core/ExitCode.h>
#include <drea/core/Option.h>

#include "integrations/help/validate.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using drea::core::App;
using drea::core::Option;

namespace {

struct AppFixture {
	char  argv0[16] = "drea-test";
	char* argv[1]   = { argv0 };
	App   app;
	AppFixture() : app( 1, argv ) {}
};

bool hasFinding( const std::vector<drea::core::Config::Finding> & findings, const std::string & code, const std::string & name )
{
	for( const auto & finding: findings ){
		if( finding.mCode == code && finding.mName == name ){
			return true;
		}
	}
	return false;
}

const drea::core::Config::Finding * getFinding( const std::vector<drea::core::Config::Finding> & findings, const std::string & code, const std::string & name )
{
	for( const auto & finding: findings ){
		if( finding.mCode == code && finding.mName == name ){
			return &finding;
		}
	}
	return nullptr;
}

std::string writeTempFile( const std::string & name, const std::string & content )
{
	const auto path = std::filesystem::temp_directory_path() / name;
	std::ofstream file( path );
	file << content;
	return path.string();
}

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

TEST_CASE( "findings reports a value that does not parse as its type", "[findings]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "port";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	fx.app.config().add( opt );

	fx.app.config().configure( { "--port", "banana" } );

	const auto findings = fx.app.config().findings();
	const auto * finding = getFinding( findings, "parse_error", "port" );
	REQUIRE( finding );
	REQUIRE( finding->mSource == "flag" );
	REQUIRE( finding->mMessage.find( "banana" ) != std::string::npos );
	// parse errors are part of the fatal subset
	REQUIRE_FALSE( fx.app.config().validate().empty() );
}

TEST_CASE( "findings reports an unknown config file key with the file name", "[findings]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "workers";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	fx.app.config().add( opt );
	const std::string path = writeTempFile( "drea-validate-unknown.yaml", "bogus: 1\nworkers: 4\n" );
	fx.app.config().setDefaultConfigFile( path );

	fx.app.config().configure( {} );

	const auto findings = fx.app.config().findings();
	const auto * finding = getFinding( findings, "unknown_key", "bogus" );
	REQUIRE( finding );
	REQUIRE( finding->mSource == "config-file" );
	REQUIRE( finding->mMessage.find( "Unknown config key" ) != std::string::npos );
	REQUIRE( finding->mMessage.find( path ) != std::string::npos );
	// unknown keys are reported by --validate but stay non-fatal in parse
	REQUIRE( fx.app.config().validate().empty() );
	REQUIRE( fx.app.config().get<int>( "workers" ) == 4 );
}

TEST_CASE( "findings reports an unreadable config file", "[findings]" )
{
	AppFixture fx;
	fx.app.config().setDefaultConfigFile( "/nonexistent/drea-validate.yaml" );

	fx.app.config().configure( {} );

	const auto findings = fx.app.config().findings();
	REQUIRE( hasFinding( findings, "file_error", "config-file" ) );
	REQUIRE( drea::core::integrations::Help::validateExitCode( findings ) == drea::core::ExitCode::NoInput );
}

TEST_CASE( "findings reports a config file that cannot be parsed", "[findings]" )
{
	AppFixture fx;
	const std::string path = writeTempFile( "drea-validate-broken.json", "{ this is not json" );
	fx.app.config().setDefaultConfigFile( path );

	fx.app.config().configure( {} );

	REQUIRE( hasFinding( fx.app.config().findings(), "parse_error", "config-file" ) );
}

TEST_CASE( "findings reports an option set through a source its scope disallows", "[findings]" )
{
	SECTION( "config-file-only option passed as a flag" ){
		AppFixture fx;
		Option opt;
		opt.mName = "db-password";
		opt.mParamName = "secret";
		opt.mType = typeid( std::string );
		opt.mScope = Option::Scope::File;
		fx.app.config().add( opt );

		fx.app.config().configure( { "--db-password", "hunter2" } );

		const auto findings = fx.app.config().findings();
		const auto * finding = getFinding( findings, "wrong_scope", "db-password" );
		REQUIRE( finding );
		REQUIRE( finding->mSource == "flag" );
		REQUIRE( finding->mMessage.find( "command line" ) != std::string::npos );
	}
	SECTION( "command-line-only option set from the config file" ){
		AppFixture fx;
		Option opt;
		opt.mName = "force";
		opt.mParamName = "mode";
		opt.mType = typeid( std::string );
		opt.mScope = Option::Scope::Line;
		fx.app.config().add( opt );
		const std::string path = writeTempFile( "drea-validate-scope.yaml", "force: always\n" );
		fx.app.config().setDefaultConfigFile( path );

		fx.app.config().configure( {} );

		const auto findings = fx.app.config().findings();
		const auto * finding = getFinding( findings, "wrong_scope", "force" );
		REQUIRE( finding );
		REQUIRE( finding->mSource == "config-file" );
	}
}

TEST_CASE( "findings reports missing arguments for a value-taking option", "[findings]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "port";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	fx.app.config().add( opt );

	fx.app.config().configure( { "--port" } );

	const auto findings = fx.app.config().findings();
	const auto * finding = getFinding( findings, "missing_params", "port" );
	REQUIRE( finding );
	REQUIRE( finding->mSource == "flag" );
}

TEST_CASE( "findings reports a requested command gated by disabled groups", "[findings]" )
{
	AppFixture fx;
	drea::core::Command cmd;
	cmd.mName = "deploy";
	cmd.mGroups = { "admin" };
	fx.app.commander().add( cmd );

	fx.app.config().configure( { } );
	fx.app.commander().configure( { "deploy" } );

	const auto findings = fx.app.config().findings();
	const auto * finding = getFinding( findings, "disabled_group", "deploy" );
	REQUIRE( finding );
	REQUIRE( finding->mMessage.find( "admin" ) != std::string::npos );
	REQUIRE( drea::core::integrations::Help::validateExitCode( findings ) == drea::core::ExitCode::ConfigError );

	// enabling the group clears the finding
	fx.app.commander().setEnabledGroups( { "admin" } );
	REQUIRE_FALSE( hasFinding( fx.app.config().findings(), "disabled_group", "deploy" ) );
}

TEST_CASE( "findings track the source that wins across several sources", "[findings]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "port";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	opt.mMin = 1;
	opt.mMax = 100;
	opt.mValues = { 50 };
	fx.app.config().add( opt );
	const std::string path = writeTempFile( "drea-validate-precedence.yaml", "port: 80\n" );
	fx.app.config().setDefaultConfigFile( path );

	SECTION( "the config file wins over the default and is in range" ){
		fx.app.config().configure( {} );
		REQUIRE( fx.app.config().get<int>( "port" ) == 80 );
		REQUIRE_FALSE( hasFinding( fx.app.config().findings(), "out_of_range", "port" ) );
	}
	SECTION( "the flag wins over the config file and carries the finding" ){
		fx.app.config().configure( { "--port", "200" } );
		REQUIRE( fx.app.config().get<int>( "port" ) == 200 );
		const auto findings = fx.app.config().findings();
		const auto * finding = getFinding( findings, "out_of_range", "port" );
		REQUIRE( finding );
		REQUIRE( finding->mSource == "flag" );
	}
	// the declared default survives resolution for later comparisons
	REQUIRE( fx.app.config().declaredDefault( "port" ).size() == 1 );
	REQUIRE( std::get<int>( fx.app.config().declaredDefault( "port" ).front() ) == 50 );
}

TEST_CASE( "findings mask values of sensitive options", "[findings][sensitive]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "api-key";
	opt.mParamName = "key";
	opt.mType = typeid( std::string );
	opt.mSensitive = true;
	opt.mChoices = { "alpha", "beta" };
	fx.app.config().add( opt );

	fx.app.config().configure( { "--api-key", "s3cr3t" } );

	const auto findings = fx.app.config().findings();
	const auto * finding = getFinding( findings, "bad_choice", "api-key" );
	REQUIRE( finding );
	REQUIRE( finding->mMessage.find( "[redacted]" ) != std::string::npos );
	REQUIRE( finding->mMessage.find( "s3cr3t" ) == std::string::npos );

	// masked in both output modes too
	std::ostringstream out, err;
	drea::core::integrations::Help::validateConfig( fx.app, true, out, err );
	drea::core::integrations::Help::validateConfig( fx.app, false, out, err );
	REQUIRE( out.str().find( "s3cr3t" ) == std::string::npos );
	REQUIRE( err.str().find( "s3cr3t" ) == std::string::npos );
	REQUIRE( out.str().find( "[redacted]" ) != std::string::npos );
	REQUIRE( err.str().find( "[redacted]" ) != std::string::npos );
}

TEST_CASE( "validateConfig emits drea-validate/1 JSON on stdout", "[validate-cli]" )
{
	AppFixture fx;
	fx.app.setName( "myapp" );
	fx.app.setVersion( "1.2.3" );
	Option opt;
	opt.mName = "color";
	opt.mParamName = "mode";
	opt.mType = typeid( std::string );
	opt.mChoices = { "auto", "always", "never" };
	fx.app.config().add( opt );

	fx.app.config().configure( { "--color", "sometimes" } );

	std::ostringstream out, err;
	const int code = drea::core::integrations::Help::validateConfig( fx.app, true, out, err );

	REQUIRE( code == drea::core::toInt( drea::core::ExitCode::DataError ) );
	REQUIRE( err.str().empty() );
	const std::string json = out.str();
	REQUIRE( json.find( "\"schema\": \"drea-validate/1\"" ) != std::string::npos );
	REQUIRE( json.find( "\"app\": \"myapp\"" ) != std::string::npos );
	REQUIRE( json.find( "\"valid\": false" ) != std::string::npos );
	REQUIRE( json.find( "\"option\": \"color\"" ) != std::string::npos );
	REQUIRE( json.find( "\"source\": \"flag\"" ) != std::string::npos );
	REQUIRE( json.find( "\"code\": \"bad_choice\"" ) != std::string::npos );
	REQUIRE( std::count( json.begin(), json.end(), '{' ) == std::count( json.begin(), json.end(), '}' ) );
	REQUIRE( std::count( json.begin(), json.end(), '[' ) == std::count( json.begin(), json.end(), ']' ) );
}

TEST_CASE( "validateConfig reports a valid configuration and exits 0", "[validate-cli]" )
{
	AppFixture fx;
	fx.app.setName( "myapp" );
	Option opt;
	opt.mName = "port";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	opt.mMin = 1;
	opt.mMax = 65535;
	opt.mValues = { 8080 };
	fx.app.config().add( opt );

	fx.app.config().configure( {} );

	SECTION( "human output goes to stderr" ){
		std::ostringstream out, err;
		const int code = drea::core::integrations::Help::validateConfig( fx.app, false, out, err );
		REQUIRE( code == drea::core::toInt( drea::core::ExitCode::Ok ) );
		REQUIRE( out.str().empty() );
		REQUIRE( err.str().find( "valid" ) != std::string::npos );
	}
	SECTION( "machine output goes to stdout" ){
		std::ostringstream out, err;
		const int code = drea::core::integrations::Help::validateConfig( fx.app, true, out, err );
		REQUIRE( code == drea::core::toInt( drea::core::ExitCode::Ok ) );
		REQUIRE( err.str().empty() );
		REQUIRE( out.str().find( "\"valid\": true" ) != std::string::npos );
		REQUIRE( out.str().find( "\"findings\": []" ) != std::string::npos );
	}
}

TEST_CASE( "validateExitCode maps finding categories to exit codes", "[validate-cli]" )
{
	using drea::core::ExitCode;
	using drea::core::integrations::Help::validateExitCode;
	using Finding = drea::core::Config::Finding;

	REQUIRE( validateExitCode( {} ) == ExitCode::Ok );
	REQUIRE( validateExitCode( { Finding{ "port", "flag", "out_of_range", "" } } ) == ExitCode::DataError );
	REQUIRE( validateExitCode( { Finding{ "port", "flag", "parse_error", "" } } ) == ExitCode::DataError );
	REQUIRE( validateExitCode( { Finding{ "token", "", "missing_required", "" } } ) == ExitCode::ConfigError );
	REQUIRE( validateExitCode( { Finding{ "bogus", "config-file", "unknown_key", "" } } ) == ExitCode::ConfigError );
	// structural beats values, an unreadable file beats everything
	REQUIRE( validateExitCode( { Finding{ "port", "flag", "out_of_range", "" },
		Finding{ "token", "", "missing_required", "" } } ) == ExitCode::ConfigError );
	REQUIRE( validateExitCode( { Finding{ "token", "", "missing_required", "" },
		Finding{ "config-file", "config-file", "file_error", "" } } ) == ExitCode::NoInput );
}

TEST_CASE( "log-flush-level rejects values outside its choices", "[validate]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();

	SECTION( "a spdlog level name passes" ){
		fx.app.config().configure( { "--log-flush-level", "err" } );
		REQUIRE( fx.app.config().validate().empty() );
	}
	SECTION( "an unknown level fails validation instead of a silent fallback" ){
		fx.app.config().configure( { "--log-flush-level", "loud" } );
		const auto findings = fx.app.config().findings();
		const auto * finding = getFinding( findings, "bad_choice", "log-flush-level" );
		REQUIRE( finding );
		REQUIRE( finding->mMessage.find( "trace, debug, info, warn, err, critical, off" ) != std::string::npos );
		REQUIRE_FALSE( fx.app.config().validate().empty() );
	}
}

TEST_CASE( "redundant detects a real source supplying the declared default", "[findings]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "port";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	opt.mValues = { 8080 };
	fx.app.config().add( opt );

	SECTION( "value only from the default is not redundant" ){
		fx.app.config().configure( {} );
		REQUIRE_FALSE( fx.app.config().redundant( "port" ) );
	}
	SECTION( "a flag repeating the default is redundant" ){
		fx.app.config().configure( { "--port", "8080" } );
		REQUIRE( fx.app.config().source( "port" ) == "flag" );
		REQUIRE( fx.app.config().redundant( "port" ) );
	}
	SECTION( "a flag with a different value is not redundant" ){
		fx.app.config().configure( { "--port", "9090" } );
		REQUIRE_FALSE( fx.app.config().redundant( "port" ) );
	}
}

TEST_CASE( "validateConfig reports the effective configuration", "[validate-cli]" )
{
	AppFixture fx;
	fx.app.setName( "myapp" );
	Option port;
	port.mName = "port";
	port.mParamName = "n";
	port.mType = typeid( int );
	port.mValues = { 8080 };
	fx.app.config().add( port );
	Option secret;
	secret.mName = "api-key";
	secret.mParamName = "key";
	secret.mType = typeid( std::string );
	secret.mSensitive = true;
	fx.app.config().add( secret );

	fx.app.config().configure( { "--port", "8080", "--api-key", "s3cr3t" } );

	SECTION( "in JSON" ){
		std::ostringstream out, err;
		const int code = drea::core::integrations::Help::validateConfig( fx.app, true, out, err );
		REQUIRE( code == drea::core::toInt( drea::core::ExitCode::Ok ) );
		const std::string json = out.str();
		REQUIRE( json.find( "\"effective\": [" ) != std::string::npos );
		REQUIRE( json.find( "\"option\": \"port\"" ) != std::string::npos );
		REQUIRE( json.find( "\"value\": [8080]" ) != std::string::npos );
		REQUIRE( json.find( "\"default\": [8080]" ) != std::string::npos );
		REQUIRE( json.find( "\"redundant\": true" ) != std::string::npos );
		REQUIRE( json.find( "\"source\": \"flag\"" ) != std::string::npos );
		REQUIRE( json.find( "\"value\": \"[redacted]\"" ) != std::string::npos );
		REQUIRE( json.find( "s3cr3t" ) == std::string::npos );
	}
	SECTION( "in the human summary" ){
		std::ostringstream out, err;
		drea::core::integrations::Help::validateConfig( fx.app, false, out, err );
		const std::string text = err.str();
		REQUIRE( text.find( "Effective configuration:" ) != std::string::npos );
		REQUIRE( text.find( "port=8080 (from flag, matches default)" ) != std::string::npos );
		REQUIRE( text.find( "api-key=[redacted] (from flag)" ) != std::string::npos );
		REQUIRE( text.find( "s3cr3t" ) == std::string::npos );
		REQUIRE( out.str().empty() );
	}
}

TEST_CASE( "addDefaults registers log-effective-config, not log-config", "[config]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();

	REQUIRE( fx.app.config().find( "log-effective-config" ) );
	REQUIRE_FALSE( fx.app.config().find( "log-config" ) );
}
