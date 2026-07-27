#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Config.h>
#include <drea/core/Option.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using drea::core::App;
using drea::core::Option;

// The config readers are picked by file extension (see Config::readConfig), and
// each format is compiled in only when its dependency was found, so the JSON and
// TOML cases are gated the same way the library is. YAML is always available.

namespace {

struct AppFixture {
	char  argv0[16] = "drea-test";
	char* argv[1]   = { argv0 };
	App   app;
	AppFixture() : app( 1, argv ) {}
};

std::string writeFile( const std::string & name, const std::string & content )
{
	const auto path = std::filesystem::temp_directory_path() / name;
	std::ofstream file( path );
	file << content;
	return path.string();
}

//! port (int, one value), round (bool toggle, on by default), tags (unlimited),
//! db.host (dotted key), force (command line only)
void declareOptions( App & app )
{
	Option port;
	port.mName = "port";
	port.mParamName = "n";
	port.mType = typeid( int );
	app.config().add( port );

	Option round;
	round.mName = "round";
	round.mType = typeid( bool );
	round.mValues = { true };
	app.config().add( round );

	Option tags;
	tags.mName = "tags";
	tags.mParamName = "tag";
	tags.mNbParams = Option::mUnlimitedParams;
	app.config().add( tags );

	Option host;
	host.mName = "db.host";
	host.mParamName = "host";
	host.mType = typeid( std::string );
	app.config().add( host );

	Option force;
	force.mName = "force";
	force.mParamName = "mode";
	force.mType = typeid( std::string );
	force.mScope = Option::Scope::Line;
	app.config().add( force );
}

bool hasFinding( const App & app, const std::string & code, const std::string & name )
{
	for( const auto & finding: app.config().findings() ){
		if( finding.mCode == code && finding.mName == name ){
			return true;
		}
	}
	return false;
}

}

TEST_CASE( "a YAML config file sets values, toggles and dotted keys", "[config-file]" )
{
	AppFixture fx;
	declareOptions( fx.app );
	fx.app.config().setDefaultConfigFile( writeFile( "drea-reader.yaml",
		"port: 8080\n"
		"round: false\n"
		"tags:\n"
		"  - a\n"
		"  - b\n"
		"db:\n"
		"  host: db.example.com\n"
		"bogus: 1\n"
		"force: always\n"
	) );

	fx.app.config().configure( {} );

	REQUIRE( fx.app.config().get<int>( "port" ) == 8080 );
	REQUIRE( fx.app.config().source( "port" ) == "config-file" );
	// a toggle set to false is off, not merely "mentioned"
	REQUIRE( fx.app.config().get<bool>( "round" ) == false );
	REQUIRE( fx.app.config().getAll<std::string>( "tags" ) == std::vector<std::string>{ "a", "b" } );
	REQUIRE( fx.app.config().get<std::string>( "db.host" ) == "db.example.com" );
	// a key matching no option is reported, non-fatally
	REQUIRE( hasFinding( fx.app, "unknown_key", "bogus" ) );
	// a command-line-only option is reported and left untouched
	REQUIRE( hasFinding( fx.app, "wrong_scope", "force" ) );
	REQUIRE_FALSE( fx.app.config().used( "force" ) );
}

TEST_CASE( "a YAML config file reports a value of the wrong type", "[config-file]" )
{
	AppFixture fx;
	declareOptions( fx.app );
	fx.app.config().setDefaultConfigFile( writeFile( "drea-reader-bad.yaml", "round: banana\n" ) );

	fx.app.config().configure( {} );

	REQUIRE( hasFinding( fx.app, "parse_error", "round" ) );
}

#ifdef ENABLE_JSON

TEST_CASE( "a JSON config file sets values, toggles and nested keys", "[config-file][json]" )
{
	AppFixture fx;
	declareOptions( fx.app );
	fx.app.config().setDefaultConfigFile( writeFile( "drea-reader.json",
		"{\n"
		"  \"port\": 8080,\n"
		"  \"round\": false,\n"
		"  \"tags\": [ \"a\", \"b\" ],\n"
		"  \"db\": { \"host\": \"db.example.com\" },\n"
		"  \"bogus\": 1,\n"
		"  \"force\": \"always\"\n"
		"}\n"
	) );

	fx.app.config().configure( {} );

	REQUIRE( fx.app.config().get<int>( "port" ) == 8080 );
	REQUIRE( fx.app.config().get<bool>( "round" ) == false );
	REQUIRE( fx.app.config().getAll<std::string>( "tags" ) == std::vector<std::string>{ "a", "b" } );
	REQUIRE( fx.app.config().get<std::string>( "db.host" ) == "db.example.com" );
	REQUIRE( hasFinding( fx.app, "unknown_key", "bogus" ) );
	REQUIRE( hasFinding( fx.app, "wrong_scope", "force" ) );
	REQUIRE_FALSE( fx.app.config().used( "force" ) );
}

TEST_CASE( "a JSON config file turning a toggle on keeps it on", "[config-file][json]" )
{
	AppFixture fx;
	declareOptions( fx.app );
	fx.app.config().find( "round" )->mValues = { false };
	fx.app.config().setDefaultConfigFile( writeFile( "drea-reader-on.json", "{ \"round\": true }\n" ) );

	fx.app.config().configure( {} );

	REQUIRE( fx.app.config().get<bool>( "round" ) == true );
}

#endif

#ifdef ENABLE_TOML

TEST_CASE( "a TOML config file sets values, toggles and table keys", "[config-file][toml]" )
{
	AppFixture fx;
	declareOptions( fx.app );
	fx.app.config().setDefaultConfigFile( writeFile( "drea-reader.toml",
		"port = 8080\n"
		"round = false\n"
		"tags = [ \"a\", \"b\" ]\n"
		"bogus = 1\n"
		"force = \"always\"\n"
		"[db]\n"
		"host = \"db.example.com\"\n"
	) );

	fx.app.config().configure( {} );

	REQUIRE( fx.app.config().get<int>( "port" ) == 8080 );
	REQUIRE( fx.app.config().get<bool>( "round" ) == false );
	REQUIRE( fx.app.config().getAll<std::string>( "tags" ) == std::vector<std::string>{ "a", "b" } );
	REQUIRE( fx.app.config().get<std::string>( "db.host" ) == "db.example.com" );
	REQUIRE( hasFinding( fx.app, "unknown_key", "bogus" ) );
	REQUIRE( hasFinding( fx.app, "wrong_scope", "force" ) );
	REQUIRE_FALSE( fx.app.config().used( "force" ) );
}

TEST_CASE( "a TOML config file turning a toggle on keeps it on", "[config-file][toml]" )
{
	AppFixture fx;
	declareOptions( fx.app );
	fx.app.config().find( "round" )->mValues = { false };
	fx.app.config().setDefaultConfigFile( writeFile( "drea-reader-on.toml", "round = true\n" ) );

	fx.app.config().configure( {} );

	REQUIRE( fx.app.config().get<bool>( "round" ) == true );
}

#endif

TEST_CASE( "--config-file is read in both accepted forms", "[config-file]" )
{
	const std::string path = writeFile( "drea-both-forms.yaml", "port: 8080\n" );

	SECTION( "split form" ){
		AppFixture fx;
		declareOptions( fx.app );
		fx.app.config().addDefaults();
		fx.app.config().configure( { "--config-file", path } );
		REQUIRE( fx.app.config().get<int>( "port" ) == 8080 );
	}
	SECTION( "inline form" ){
		AppFixture fx;
		declareOptions( fx.app );
		fx.app.config().addDefaults();
		// the loader that runs before the general flag parsing used to scan for
		// the split form only, so --config-file=path read nothing at all
		fx.app.config().configure( { "--config-file=" + path } );
		REQUIRE( fx.app.config().get<int>( "port" ) == 8080 );
	}
}

TEST_CASE( "a config file is not read once config-file has been removed", "[config-file]" )
{
	AppFixture fx;
	declareOptions( fx.app );
	fx.app.config().addDefaults();
	const std::string path = writeFile( "drea-removed-option.yaml", "port: 9090\n" );

	fx.app.config().remove( "config-file" );
	fx.app.config().configure( { "--config-file", path } );

	// Config::remove drops the pre-scan too: the flag is unknown now
	REQUIRE_FALSE( fx.app.config().used( "port" ) );
	REQUIRE( hasFinding( fx.app, "unknown_key", "config-file" ) );
}

TEST_CASE( "the default config file is still read when no flag is given", "[config-file]" )
{
	AppFixture fx;
	declareOptions( fx.app );
	fx.app.config().setDefaultConfigFile( writeFile( "drea-default-file.yaml", "port: 7070\n" ) );

	fx.app.config().configure( {} );

	REQUIRE( fx.app.config().get<int>( "port" ) == 7070 );
}

#ifdef ENABLE_JSON

TEST_CASE( "JSON numbers keep the precision a double holds", "[config-file][json]" )
{
	AppFixture fx;
	Option ratio;
	ratio.mName = "ratio";
	ratio.mParamName = "x";
	ratio.mType = typeid( double );
	fx.app.config().add( ratio );
	Option big;
	big.mName = "big";
	big.mParamName = "x";
	big.mType = typeid( double );
	fx.app.config().add( big );
	Option count;
	count.mName = "count";
	count.mParamName = "n";
	count.mType = typeid( int );
	fx.app.config().add( count );

	fx.app.config().setDefaultConfigFile( writeFile( "drea-precision.json",
		"{ \"ratio\": 0.12345678901234567, \"big\": 1e40, \"count\": 2147483647 }\n" ) );

	fx.app.config().configure( {} );

	// read as float, 0.12345678901234567 became 0.12345679 and 1e40 became inf
	REQUIRE( fx.app.config().get<double>( "ratio" ) == 0.12345678901234566 );
	REQUIRE( fx.app.config().get<double>( "big" ) > 1e39 );
	REQUIRE( std::isfinite( fx.app.config().get<double>( "big" ) ) );
	REQUIRE( fx.app.config().get<int>( "count" ) == 2147483647 );
	REQUIRE( fx.app.config().findings().empty() );
}

#endif
