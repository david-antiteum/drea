#include <catch2/catch_test_macros.hpp>

#include <drea/core/Command.h>

using drea::core::Command;

TEST_CASE( "Command::numberOfParams returns 0 when no param name", "[command]" )
{
	Command cmd;
	cmd.mName = "noop";
	cmd.mNbParams = 3;
	REQUIRE( cmd.numberOfParams() == 0 );
}

TEST_CASE( "Command::numberOfParams returns mNbParams when param name set", "[command]" )
{
	Command cmd;
	cmd.mName = "copy";
	cmd.mParamName = "src dst";
	cmd.mNbParams = 2;
	REQUIRE( cmd.numberOfParams() == 2 );
}

TEST_CASE( "Command::numberOfParams supports unlimited", "[command]" )
{
	Command cmd;
	cmd.mName = "sum";
	cmd.mParamName = "values";
	cmd.mNbParams = Command::mUnlimitedParams;
	REQUIRE( cmd.numberOfParams() == Command::mUnlimitedParams );
}

TEST_CASE( "Command::nameOfParamsForHelp marks the names above the minimum as optional", "[command]" )
{
	Command cmd;
	cmd.mParamName = "src dst";
	// mNbParams defaults to 1: only the first name is ever read
	REQUIRE( cmd.nameOfParamsForHelp() == "<src> [dst]" );
}

TEST_CASE( "Command::nameOfParamsForHelp handles single name", "[command]" )
{
	Command cmd;
	cmd.mParamName = "file";
	REQUIRE( cmd.nameOfParamsForHelp() == "<file>" );
}

TEST_CASE( "Command::nameOfParamsForHelp marks unlimited params with an ellipsis", "[command]" )
{
	Command cmd;
	cmd.mParamName = "value";
	cmd.mNbParams = Command::mUnlimitedParams;
	REQUIRE( cmd.nameOfParamsForHelp() == "[value]..." );

	cmd.mMinParams = 1;
	REQUIRE( cmd.nameOfParamsForHelp() == "<value>..." );
}

TEST_CASE( "Command::minParams defaults to numberOfParams when mMinParams unset", "[command]" )
{
	Command cmd;
	cmd.mParamName = "src dst";
	cmd.mNbParams = 2;
	REQUIRE( cmd.minParams() == 2 );
	REQUIRE( cmd.maxParams() == 2 );
}

TEST_CASE( "Command::minParams honors mMinParams when set", "[command]" )
{
	Command cmd;
	cmd.mParamName = "id";
	cmd.mNbParams = 1;
	cmd.mMinParams = 0;
	REQUIRE( cmd.minParams() == 0 );
	REQUIRE( cmd.maxParams() == 1 );
}

TEST_CASE( "Command::nameOfParamsForHelp renders required vs optional when mMinParams set", "[command]" )
{
	Command cmd;
	cmd.mParamName = "src dst";
	cmd.mNbParams = 2;
	cmd.mMinParams = 1;
	REQUIRE( cmd.nameOfParamsForHelp() == "<src> [dst]" );
}

TEST_CASE( "Command::nameOfParamsForHelp marks every name required without mMinParams", "[command]" )
{
	Command cmd;
	cmd.mParamName = "src dst";
	cmd.mNbParams = 2;
	REQUIRE( cmd.nameOfParamsForHelp() == "<src> <dst>" );
}

TEST_CASE( "Command::mHidden defaults to false", "[command]" )
{
	Command cmd;
	REQUIRE( cmd.mHidden == false );
}

