#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Config.h>
#include <drea/core/Option.h>

#include "integrations/help/describe.h"

#include <filesystem>
#include <fstream>
#include <sstream>
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

TEST_CASE( "Config --no-X negates bool option with default true", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "dry-run";
	opt.mType = typeid( bool );
	opt.mValues = { true };
	fx.app.config().add( opt );

	fx.app.config().configure( { "--no-dry-run" } );

	REQUIRE( fx.app.config().used( "dry-run" ) );
	REQUIRE( fx.app.config().get<bool>( "dry-run" ) == false );
}

TEST_CASE( "Config --no-X ignored for non-bool options", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "file";
	opt.mParamName = "path";
	opt.mType = typeid( std::string );
	fx.app.config().add( opt );

	fx.app.config().configure( { "--no-file" } );

	REQUIRE_FALSE( fx.app.config().used( "file" ) );
}

TEST_CASE( "Config --X followed by --no-X yields false", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "verbose";
	opt.mType = typeid( bool );
	opt.mValues = { false };
	fx.app.config().add( opt );

	fx.app.config().configure( { "--verbose", "--no-verbose" } );

	REQUIRE( fx.app.config().used( "verbose" ) );
	REQUIRE( fx.app.config().get<bool>( "verbose" ) == false );
}

TEST_CASE( "Config --X enables bool option with no default", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "verbose";
	opt.mType = typeid( bool );
	fx.app.config().add( opt );

	fx.app.config().configure( { "--verbose" } );

	REQUIRE( fx.app.config().used( "verbose" ) );
	REQUIRE( fx.app.config().get<bool>( "verbose" ) == true );
}

TEST_CASE( "Config --X enables bool option that defaulted to false", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "trace";
	opt.mType = typeid( bool );
	opt.mValues = { false };
	fx.app.config().add( opt );

	fx.app.config().configure( { "--trace" } );

	REQUIRE( fx.app.config().used( "trace" ) );
	REQUIRE( fx.app.config().get<bool>( "trace" ) == true );
}

TEST_CASE( "Config::set marks option as used", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "log-file";
	opt.mParamName = "file";
	opt.mType = typeid( std::string );
	fx.app.config().add( opt );

	REQUIRE_FALSE( fx.app.config().used( "log-file" ) );

	fx.app.config().set( "log-file", "/tmp/x.log" );

	REQUIRE( fx.app.config().used( "log-file" ) );
	REQUIRE( fx.app.config().get<std::string>( "log-file" ) == "/tmp/x.log" );
}

TEST_CASE( "Config --opt=value sets string value", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "log-file";
	opt.mParamName = "file";
	opt.mType = typeid( std::string );
	fx.app.config().add( opt );

	fx.app.config().configure( { "--log-file=/tmp/x.log" } );

	REQUIRE( fx.app.config().used( "log-file" ) );
	REQUIRE( fx.app.config().get<std::string>( "log-file" ) == "/tmp/x.log" );
}

TEST_CASE( "Config --opt=value accepts hyphen-leading values", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "threshold";
	opt.mParamName = "x";
	opt.mType = typeid( double );
	fx.app.config().add( opt );

	fx.app.config().configure( { "--threshold=-0.5" } );

	REQUIRE( fx.app.config().used( "threshold" ) );
	REQUIRE( fx.app.config().get<double>( "threshold" ) == -0.5 );
}

TEST_CASE( "Config --opt=value accepts integer with sign", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "offset";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	fx.app.config().add( opt );

	fx.app.config().configure( { "--offset=-1" } );

	REQUIRE( fx.app.config().get<int>( "offset" ) == -1 );
}

TEST_CASE( "Config --opt= sets empty string", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "label";
	opt.mParamName = "text";
	opt.mType = typeid( std::string );
	fx.app.config().add( opt );

	fx.app.config().configure( { "--label=" } );

	REQUIRE( fx.app.config().used( "label" ) );
	REQUIRE( fx.app.config().get<std::string>( "label" ) == "" );
}

TEST_CASE( "Config::remove erases an option from the registry", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "drop-me";
	opt.mType = typeid( std::string );
	fx.app.config().add( opt );

	REQUIRE( fx.app.config().find( "drop-me" ) );

	fx.app.config().remove( "drop-me" );

	REQUIRE_FALSE( fx.app.config().find( "drop-me" ) );
}

TEST_CASE( "Config::remove also clears the used flag", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "drop-me";
	opt.mType = typeid( bool );
	opt.mNbParams = 0;
	fx.app.config().add( opt );

	fx.app.config().registerUse( "drop-me" );
	REQUIRE( fx.app.config().used( "drop-me" ) );

	fx.app.config().remove( "drop-me" );
	REQUIRE_FALSE( fx.app.config().used( "drop-me" ) );
}

TEST_CASE( "Config::remove on unknown name is a no-op", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "kept";
	fx.app.config().add( opt );

	fx.app.config().remove( "missing" );

	REQUIRE( fx.app.config().find( "kept" ) );
}

TEST_CASE( "addDefaults marks its options as predefined", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "color";
	opt.mParamName = "mode";
	opt.mType = typeid( std::string );
	fx.app.config().add( opt );

	fx.app.config().addDefaults();

	REQUIRE( fx.app.config().find( "verbose" )->mPredefined );
	REQUIRE( fx.app.config().find( "log-file" )->mPredefined );
	REQUIRE( fx.app.config().find( "config-file" )->mPredefined );
	REQUIRE_FALSE( fx.app.config().find( "color" )->mPredefined );
}

TEST_CASE( "Config::remove drops a default added by addDefaults", "[config]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();

	if( fx.app.config().find( "graylog-host" ) ){
		fx.app.config().remove( "graylog-host" );
		REQUIRE_FALSE( fx.app.config().find( "graylog-host" ) );
	}else{
		// Drea was built without ENABLE_REST_USE; nothing to remove.
		SUCCEED();
	}
}

namespace {

void setEnvVar( const char * name, const char * value )
{
#ifdef WIN32
	_putenv_s( name, value );
#else
	setenv( name, value, 1 );
#endif
}

void unsetEnvVar( const char * name )
{
#ifdef WIN32
	_putenv_s( name, "" );
#else
	unsetenv( name );
#endif
}

}

TEST_CASE( "Config reads env var with '-' in option name mapped to '_'", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "config-file";
	opt.mParamName = "path";
	opt.mType = typeid( std::string );
	fx.app.config().add( opt );
	fx.app.config().setEnvPrefix( "DREATEST" );

	setEnvVar( "DREATEST_config_file", "/etc/config.json" );
	fx.app.config().configure( {} );
	unsetEnvVar( "DREATEST_config_file" );

	REQUIRE( fx.app.config().used( "config-file" ) );
	REQUIRE( fx.app.config().get<std::string>( "config-file" ) == "/etc/config.json" );
	REQUIRE( fx.app.config().source( "config-file" ) == "environment" );
}

TEST_CASE( "Config reads env var with '.' in option name mapped to '_'", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "db.host";
	opt.mParamName = "host";
	opt.mType = typeid( std::string );
	fx.app.config().add( opt );
	fx.app.config().setEnvPrefix( "DREATEST" );

	setEnvVar( "DREATEST_db_host", "localhost" );
	fx.app.config().configure( {} );
	unsetEnvVar( "DREATEST_db_host" );

	REQUIRE( fx.app.config().get<std::string>( "db.host" ) == "localhost" );
}

TEST_CASE( "Config still reads env var matching option name exactly", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "workers";
	opt.mParamName = "count";
	opt.mType = typeid( int );
	fx.app.config().add( opt );
	fx.app.config().setEnvPrefix( "DREATEST" );

	setEnvVar( "DREATEST_workers", "8" );
	fx.app.config().configure( {} );
	unsetEnvVar( "DREATEST_workers" );

	REQUIRE( fx.app.config().get<int>( "workers" ) == 8 );
}

TEST_CASE( "Config reads all-uppercase env var spelling", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "config-file";
	opt.mParamName = "path";
	opt.mType = typeid( std::string );
	fx.app.config().add( opt );
	fx.app.config().setEnvPrefix( "DREATEST" );

	setEnvVar( "DREATEST_CONFIG_FILE", "/etc/upper.json" );
	fx.app.config().configure( {} );
	unsetEnvVar( "DREATEST_CONFIG_FILE" );

	REQUIRE( fx.app.config().get<std::string>( "config-file" ) == "/etc/upper.json" );
}

TEST_CASE( "env vars are ignored for command-line scoped options", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "burst";
	opt.mParamName = "n";
	opt.mType = typeid( int );
	opt.mScope = Option::Scope::Line;
	fx.app.config().add( opt );
	fx.app.config().setEnvPrefix( "DREATEST" );

	setEnvVar( "DREATEST_burst", "5" );
	fx.app.config().configure( {} );
	unsetEnvVar( "DREATEST_burst" );

	REQUIRE_FALSE( fx.app.config().used( "burst" ) );
}

namespace {

std::string writeConfigFile( const std::string & name, const std::string & content )
{
	const auto path = std::filesystem::temp_directory_path() / name;
	std::ofstream file( path );
	file << content;
	return path.string();
}

}

TEST_CASE( "a config file sets the value of a bool flag", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "round";
	opt.mType = typeid( bool );
	opt.mValues = { true };
	fx.app.config().add( opt );
	fx.app.config().setDefaultConfigFile( writeConfigFile( "drea-config-bool.yaml", "round: false\n" ) );

	fx.app.config().configure( {} );

	REQUIRE( fx.app.config().used( "round" ) );
	REQUIRE( fx.app.config().get<bool>( "round" ) == false );
	REQUIRE( fx.app.config().source( "round" ) == "config-file" );
}

TEST_CASE( "a config file enabling a bool flag keeps it true", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "round";
	opt.mType = typeid( bool );
	opt.mValues = { false };
	fx.app.config().add( opt );
	fx.app.config().setDefaultConfigFile( writeConfigFile( "drea-config-bool-on.yaml", "round: yes\n" ) );

	fx.app.config().configure( {} );

	REQUIRE( fx.app.config().get<bool>( "round" ) == true );
}

TEST_CASE( "a command line flag beats the config file for a bool", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "round";
	opt.mType = typeid( bool );
	opt.mValues = { false };
	fx.app.config().add( opt );
	fx.app.config().setDefaultConfigFile( writeConfigFile( "drea-config-bool-flag.yaml", "round: false\n" ) );

	fx.app.config().configure( { "--round" } );

	REQUIRE( fx.app.config().get<bool>( "round" ) == true );
	REQUIRE( fx.app.config().source( "round" ) == "flag" );
}

TEST_CASE( "a bad bool value in a config file is a parse_error finding", "[config]" )
{
	AppFixture fx;
	Option opt;
	opt.mName = "round";
	opt.mType = typeid( bool );
	opt.mValues = { true };
	fx.app.config().add( opt );
	fx.app.config().setDefaultConfigFile( writeConfigFile( "drea-config-bool-bad.yaml", "round: banana\n" ) );

	fx.app.config().configure( {} );

	bool found = false;
	for( const auto & finding: fx.app.config().findings() ){
		if( finding.mName == "round" && finding.mCode == "parse_error" ){
			found = true;
		}
	}
	REQUIRE( found );
}

TEST_CASE( "a yml option taking no value is inferred to be a bool", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: round\n"
		"    description: round the result\n"
	);

	auto opt = fx.app.config().find( "round" );
	REQUIRE( opt );
	REQUIRE( opt->mType == typeid( bool ) );
	REQUIRE( opt->numberOfParams() == 0 );

	// the whole point: the flag carries a value now
	fx.app.config().configure( { "--round" } );
	REQUIRE( fx.app.config().get<bool>( "round" ) == true );
}

TEST_CASE( "an inferred bool honours --no-X", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: round\n"
		"    description: round the result\n"
	);

	fx.app.config().configure( { "--no-round" } );

	REQUIRE( fx.app.config().used( "round" ) );
	REQUIRE( fx.app.config().get<bool>( "round" ) == false );
}

TEST_CASE( "an inferred bool reads its value from a config file", "[config]" )
{
	const std::string yml =
		"app: drea-test\n"
		"options:\n"
		"  - option: round\n"
		"    description: round the result\n";

	SECTION( "on" ){
		AppFixture fx;
		fx.app.parse( yml );
		fx.app.config().setDefaultConfigFile( writeConfigFile( "drea-inferred-bool-on.yaml", "round: true\n" ) );

		fx.app.config().configure( {} );

		REQUIRE( fx.app.config().get<bool>( "round" ) == true );
	}
	SECTION( "off" ){
		AppFixture fx;
		fx.app.parse( yml );
		fx.app.config().setDefaultConfigFile( writeConfigFile( "drea-inferred-bool-off.yaml", "round: false\n" ) );

		fx.app.config().configure( {} );

		REQUIRE( fx.app.config().get<bool>( "round" ) == false );
	}
}

TEST_CASE( "an explicit type: string is not turned into a bool", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: label\n"
		"    description: a label the app sets\n"
		"    type: string\n"
	);

	auto opt = fx.app.config().find( "label" );
	REQUIRE( opt );
	REQUIRE( opt->mType == typeid( std::string ) );
}

TEST_CASE( "scope: none options keep the default type so the app can set them", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: build-id\n"
		"    description: set by the app at startup\n"
		"    scope: none\n"
	);

	auto opt = fx.app.config().find( "build-id" );
	REQUIRE( opt );
	REQUIRE( opt->mType == typeid( std::string ) );

	fx.app.config().set( "build-id", "abc123" );
	REQUIRE( fx.app.config().get<std::string>( "build-id" ) == "abc123" );
}

TEST_CASE( "an option with params-names keeps the default string type", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: log-tag\n"
		"    description: tag for the logs\n"
		"    params-names: tag\n"
	);

	auto opt = fx.app.config().find( "log-tag" );
	REQUIRE( opt );
	REQUIRE( opt->mType == typeid( std::string ) );
	REQUIRE( opt->numberOfParams() == 1 );
}

TEST_CASE( "an action option refuses --no-X instead of triggering itself", "[config]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();

	fx.app.config().configure( { "--no-help" } );

	// the whole point: negating an action must not read as "show the help"
	REQUIRE_FALSE( fx.app.config().used( "help" ) );
	REQUIRE( fx.app.config().get<bool>( "help" ) == false );

	bool found = false;
	for( const auto & finding: fx.app.config().findings() ){
		if( finding.mName == "help" && finding.mCode == "not_negatable" ){
			found = true;
		}
	}
	REQUIRE( found );
	// non-fatal: the app keeps running, like an unknown argument
	REQUIRE( fx.app.config().validate().empty() );
}

TEST_CASE( "the actions among the default options are not negatable", "[config]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();

	for( const char * action: { "help", "version", "validate" } ){
		REQUIRE_FALSE( fx.app.config().find( action )->mNegatable );
	}
	// toggles keep their negation
	for( const char * toggle: { "verbose", "json", "log-redact", "log-effective-config" } ){
		REQUIRE( fx.app.config().find( toggle )->mNegatable );
	}
}

TEST_CASE( "a toggle stays negatable and --no-X still wins", "[config]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();

	fx.app.config().configure( { "--no-log-redact" } );

	REQUIRE( fx.app.config().used( "log-redact" ) );
	REQUIRE( fx.app.config().get<bool>( "log-redact" ) == false );
}

TEST_CASE( "negatable: false can be declared in yml", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: reset-db\n"
		"    description: recreate the schema and quit\n"
		"    negatable: false\n"
	);

	auto opt = fx.app.config().find( "reset-db" );
	REQUIRE( opt );
	REQUIRE( opt->mType == typeid( bool ) );	// inferred: takes no value
	REQUIRE_FALSE( opt->mNegatable );

	fx.app.config().configure( { "--no-reset-db" } );
	REQUIRE_FALSE( fx.app.config().used( "reset-db" ) );
}

TEST_CASE( "describe reports negatable only when false", "[config]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();

	std::ostringstream out;
	drea::core::integrations::Help::describe( fx.app, out );
	const std::string json = out.str();

	REQUIRE( json.find( "\"negatable\": false" ) != std::string::npos );
	REQUIRE( json.find( "\"negatable\": true" ) == std::string::npos );
}

TEST_CASE( "values: defaults are stored with the declared type", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: ports\n"
		"    description: ports to listen on\n"
		"    params-names: port\n"
		"    params: unlimited\n"
		"    type: int\n"
		"    values: [80, 443]\n"
	);

	// used to be kept as strings, so these threw std::bad_variant_access
	REQUIRE( fx.app.config().get<int>( "ports" ) == 80 );
	REQUIRE( fx.app.config().getAll<int>( "ports" ) == std::vector<int>{ 80, 443 } );
	REQUIRE( fx.app.config().find( "ports" )->toString( fx.app.config().find( "ports" )->mValues.front() ) == "80" );
}

TEST_CASE( "values: declared before the type are still typed", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: ratios\n"
		"    description: ratios\n"
		"    params-names: ratio\n"
		"    params: unlimited\n"
		"    values: [0.5, 1.5]\n"
		"    type: double\n"
	);

	REQUIRE( fx.app.config().getAll<double>( "ratios" ) == std::vector<double>{ 0.5, 1.5 } );
}

TEST_CASE( "values: without a type stay strings", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: hosts\n"
		"    description: hosts\n"
		"    params-names: host\n"
		"    params: unlimited\n"
		"    values: [a, b]\n"
	);

	REQUIRE( fx.app.config().find( "hosts" )->mType == typeid( std::string ) );
	REQUIRE( fx.app.config().getAll<std::string>( "hosts" ) == std::vector<std::string>{ "a", "b" } );
}

TEST_CASE( "values: on a bool option are parsed as booleans", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: flags\n"
		"    description: flags\n"
		"    params-names: flag\n"
		"    params: unlimited\n"
		"    type: bool\n"
		"    values: [true, false]\n"
	);

	REQUIRE( fx.app.config().getAll<bool>( "flags" ) == std::vector<bool>{ true, false } );
}

TEST_CASE( "an option with values: is not inferred to be a bool", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: mode\n"
		"    description: no params-names, but a declared default\n"
		"    values: [fast]\n"
	);

	auto opt = fx.app.config().find( "mode" );
	REQUIRE( opt );
	REQUIRE( opt->mType == typeid( std::string ) );
	REQUIRE( fx.app.config().get<std::string>( "mode" ) == "fast" );
}

TEST_CASE( "params: unlimited consumes every value given", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: tags\n"
		"    description: tags to apply\n"
		"    params-names: tag\n"
		"    params: unlimited\n"
	);

	fx.app.config().configure( { "--tags", "a", "b", "c" } );

	// the loop used to compare against the negative mUnlimitedParams sentinel
	// and consume nothing at all
	REQUIRE( fx.app.config().getAll<std::string>( "tags" ) == std::vector<std::string>{ "a", "b", "c" } );
}

TEST_CASE( "params: unlimited stops at the next option", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: tags\n"
		"    description: tags\n"
		"    params-names: tag\n"
		"    params: unlimited\n"
		"  - option: label\n"
		"    description: label\n"
		"    params-names: text\n"
	);

	fx.app.config().configure( { "--tags", "a", "b", "--label", "x" } );

	REQUIRE( fx.app.config().getAll<std::string>( "tags" ) == std::vector<std::string>{ "a", "b" } );
	REQUIRE( fx.app.config().get<std::string>( "label" ) == "x" );
}

TEST_CASE( "params: unlimited accepts the inline form", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: tags\n"
		"    description: tags\n"
		"    params-names: tag\n"
		"    params: unlimited\n"
	);

	// used to warn "Flag tags does not take a value" and drop it
	fx.app.config().configure( { "--tags=a" } );

	REQUIRE( fx.app.config().getAll<std::string>( "tags" ) == std::vector<std::string>{ "a" } );
}

TEST_CASE( "params: unlimited is typed like any other option", "[config]" )
{
	AppFixture fx;
	fx.app.parse(
		"app: drea-test\n"
		"options:\n"
		"  - option: ports\n"
		"    description: ports\n"
		"    params-names: port\n"
		"    params: unlimited\n"
		"    type: int\n"
	);

	fx.app.config().configure( { "--ports", "80", "443" } );

	REQUIRE( fx.app.config().getAll<int>( "ports" ) == std::vector<int>{ 80, 443 } );
}

TEST_CASE( "Option::takesValues covers the unlimited sentinel", "[config]" )
{
	Option unlimited;
	unlimited.mName = "tags";
	unlimited.mParamName = "tag";
	unlimited.mNbParams = Option::mUnlimitedParams;
	REQUIRE( unlimited.unlimitedParams() );
	REQUIRE( unlimited.takesValues() );
	REQUIRE_FALSE( unlimited.numberOfParams() > 0 );	// the trap this replaces

	Option single;
	single.mName = "label";
	single.mParamName = "text";
	REQUIRE_FALSE( single.unlimitedParams() );
	REQUIRE( single.takesValues() );

	Option flag;
	flag.mName = "verbose";
	flag.mType = typeid( bool );
	REQUIRE_FALSE( flag.unlimitedParams() );
	REQUIRE_FALSE( flag.takesValues() );
}
