#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Config.h>

#include <integrations/logs/json_formatter.h>

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <string>

using drea::core::App;
using drea::core::integrations::logs::json_lines_formatter;

namespace {

struct AppFixture {
	char  argv0[16] = "drea-test";
	char* argv[1]   = { argv0 };
	App   app;
	AppFixture() : app( 1, argv ) {}
};

std::string format( spdlog::level::level_enum level, const std::string & payload, const std::string & loggerName = "test" )
{
	json_lines_formatter		formatter;
	spdlog::memory_buf_t		dest;
	spdlog::details::log_msg	msg( loggerName, level, payload );

	formatter.format( msg, dest );
	return std::string( dest.data(), dest.size() );
}

}

TEST_CASE( "JSON formatter emits one object per line with the expected fields", "[logger]" )
{
	const std::string line = format( spdlog::level::info, "hello" );

	REQUIRE( line.rfind( "{\"timestamp\":\"", 0 ) == 0 );
	REQUIRE( line.find( "\",\"level\":\"info\",\"logger\":\"test\",\"msg\":\"hello\"}" ) != std::string::npos );
	REQUIRE( line.back() == '\n' );
}

TEST_CASE( "JSON formatter timestamp is ISO 8601 with offset", "[logger]" )
{
	const std::string line = format( spdlog::level::info, "x" );
	const std::string stamp = line.substr( 14, line.find( "\",\"level\"" ) - 14 );

	// 2026-07-06T12:34:56.789+02:00 (or ...Z / +0000 variants by platform)
	REQUIRE( stamp.size() >= 23 );
	REQUIRE( stamp[4] == '-' );
	REQUIRE( stamp[10] == 'T' );
	REQUIRE( stamp[19] == '.' );
}

TEST_CASE( "JSON formatter escapes quotes, backslashes and control characters", "[logger]" )
{
	const std::string line = format( spdlog::level::warn, "a \"b\" c:\\d\ne\tf\x01g" );

	REQUIRE( line.find( "\"msg\":\"a \\\"b\\\" c:\\\\d\\ne\\tf\\u0001g\"" ) != std::string::npos );
}

TEST_CASE( "JSON formatter reports the level name", "[logger]" )
{
	REQUIRE( format( spdlog::level::err, "x" ).find( "\"level\":\"error\"" ) != std::string::npos );
	REQUIRE( format( spdlog::level::debug, "x" ).find( "\"level\":\"debug\"" ) != std::string::npos );
}

TEST_CASE( "setupLogger flushes on warn by default", "[logger]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();
	fx.app.config().configure( {} );

	auto logger = fx.app.config().setupLogger();

	REQUIRE( logger->flush_level() == spdlog::level::warn );
}

TEST_CASE( "log-flush-level overrides the flush policy", "[logger]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();
	fx.app.config().configure( { "--log-flush-level", "trace" } );

	auto logger = fx.app.config().setupLogger();

	REQUIRE( logger->flush_level() == spdlog::level::trace );
}

TEST_CASE( "log-flush-level off disables flush-on-level", "[logger]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();
	fx.app.config().configure( { "--log-flush-level", "off" } );

	auto logger = fx.app.config().setupLogger();

	REQUIRE( logger->flush_level() == spdlog::level::off );
}

TEST_CASE( "unknown log-flush-level falls back to warn", "[logger]" )
{
	AppFixture fx;
	fx.app.config().addDefaults();
	fx.app.config().configure( { "--log-flush-level", "loud" } );

	auto logger = fx.app.config().setupLogger();

	REQUIRE( logger->flush_level() == spdlog::level::warn );
}

TEST_CASE( "file sink writes JSON lines", "[logger]" )
{
	AppFixture fx;
	const auto logFile = std::filesystem::temp_directory_path() / "drea-logger-test.log";

	std::filesystem::remove( logFile );
	fx.app.config().addDefaults();
	fx.app.config().configure( { "--log-file", logFile.string() } );

	auto logger = fx.app.config().setupLogger();
	logger->info( "json goes to the file" );
	logger->flush();

	std::ifstream	in( logFile );
	std::string		line;
	REQUIRE( std::getline( in, line ) );
	REQUIRE( line.front() == '{' );
	REQUIRE( line.find( "\"msg\":\"json goes to the file\"" ) != std::string::npos );
	REQUIRE( line.back() == '}' );

	in.close();
	std::filesystem::remove( logFile );
}
