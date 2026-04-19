#include <catch2/catch_test_macros.hpp>

#include <drea/core/Option.h>

using drea::core::Option;
using drea::core::OptionValue;

TEST_CASE( "Option::numberOfParams returns 0 when no param name", "[option]" )
{
	Option opt;
	opt.mName = "verbose";
	REQUIRE( opt.numberOfParams() == 0 );
}

TEST_CASE( "Option::numberOfParams returns mNbParams when param name set", "[option]" )
{
	Option opt;
	opt.mName = "file";
	opt.mParamName = "path";
	opt.mNbParams = 1;
	REQUIRE( opt.numberOfParams() == 1 );
}

TEST_CASE( "Option scope Both appears in both line and file help", "[option]" )
{
	Option opt;
	opt.mScope = Option::Scope::Both;
	REQUIRE( opt.helpInLine() );
	REQUIRE_FALSE( opt.helpInFileOnly() );
}

TEST_CASE( "Option scope Line appears in command line help only", "[option]" )
{
	Option opt;
	opt.mScope = Option::Scope::Line;
	REQUIRE( opt.helpInLine() );
	REQUIRE_FALSE( opt.helpInFileOnly() );
}

TEST_CASE( "Option scope File appears in config file help only", "[option]" )
{
	Option opt;
	opt.mScope = Option::Scope::File;
	REQUIRE_FALSE( opt.helpInLine() );
	REQUIRE( opt.helpInFileOnly() );
}

TEST_CASE( "Option scope None hides from all help", "[option]" )
{
	Option opt;
	opt.mScope = Option::Scope::None;
	REQUIRE_FALSE( opt.helpInLine() );
	REQUIRE_FALSE( opt.helpInFileOnly() );
}

TEST_CASE( "Option::toString/fromString round-trip int", "[option]" )
{
	Option opt;
	opt.mName = "count";
	opt.mType = typeid( int );

	OptionValue v = opt.fromString( "42" );
	REQUIRE( std::get<int>( v ) == 42 );
	REQUIRE( opt.toString( v ) == "42" );
}

TEST_CASE( "Option::toString/fromString round-trip string", "[option]" )
{
	Option opt;
	opt.mName = "name";
	opt.mType = typeid( std::string );

	OptionValue v = opt.fromString( "hello" );
	REQUIRE( std::get<std::string>( v ) == "hello" );
	REQUIRE( opt.toString( v ) == "hello" );
}

TEST_CASE( "Option::toString/fromString round-trip bool true", "[option]" )
{
	Option opt;
	opt.mName = "flag";
	opt.mType = typeid( bool );

	OptionValue v = opt.fromString( "true" );
	REQUIRE( std::get<bool>( v ) == true );
}

TEST_CASE( "Option::mSensitive defaults to false", "[option]" )
{
	Option opt;
	REQUIRE( opt.mSensitive == false );
}
