#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Config.h>
#include <drea/core/Option.h>

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
