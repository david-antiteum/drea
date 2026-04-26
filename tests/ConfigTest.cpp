#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Config.h>
#include <drea/core/Option.h>

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
