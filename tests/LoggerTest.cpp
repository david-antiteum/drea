#include <catch2/catch_test_macros.hpp>

#include <drea/core/App.h>
#include <drea/core/Config.h>
#include <drea/log/Logger.h>
#include <drea/log/Redacted.h>

#include <integrations/logs/json_formatter.h>
#include <integrations/logs/text_fields_formatter.h>

#include <spdlog/spdlog.h>
#include <drea/log/Mdc.h>

#include <thread>
#include <spdlog/sinks/ostream_sink.h>

#include <filesystem>
#include <fstream>
#include <sstream>
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

// spdlog logger writing JSON lines into a string, wrapped by drea::log::Logger
struct CapturedLogger {
	std::ostringstream     out;
	spdlog::logger         raw;
	drea::log::Logger      logger;

	CapturedLogger()
		: raw( "test", std::make_shared<spdlog::sinks::ostream_sink_st>( out ) )
		, logger( raw )
	{
		raw.set_formatter( std::make_unique<json_lines_formatter>() );
	}

	std::string text() { return out.str(); }
};

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

TEST_CASE( "JSON formatter emits MDC entries as top-level fields", "[logger]" )
{
	drea::log::mdc::clear();
	drea::log::mdc::put( "session", "abc-123" );
	drea::log::mdc::put( "user", "dave" );

	const std::string line = format( spdlog::level::info, "hello" );

	drea::log::mdc::clear();
	REQUIRE( line.find( "\"logger\":\"test\",\"session\":\"abc-123\",\"user\":\"dave\",\"msg\":\"hello\"" ) != std::string::npos );
}

TEST_CASE( "JSON formatter escapes MDC keys and values", "[logger]" )
{
	drea::log::mdc::clear();
	drea::log::mdc::put( "we\"ird", "a\\b\nc" );

	const std::string line = format( spdlog::level::info, "x" );

	drea::log::mdc::clear();
	REQUIRE( line.find( "\"we\\\"ird\":\"a\\\\b\\nc\"" ) != std::string::npos );
}

TEST_CASE( "JSON formatter skips reserved MDC keys", "[logger]" )
{
	drea::log::mdc::clear();
	drea::log::mdc::put( "level", "hacked" );
	drea::log::mdc::put( "msg", "hacked" );
	drea::log::mdc::put( "ok", "kept" );

	const std::string line = format( spdlog::level::info, "hello" );

	drea::log::mdc::clear();
	REQUIRE( line.find( "hacked" ) == std::string::npos );
	REQUIRE( line.find( "\"ok\":\"kept\"" ) != std::string::npos );
}

TEST_CASE( "JSON formatter output is unchanged with an empty MDC", "[logger]" )
{
	drea::log::mdc::clear();

	const std::string line = format( spdlog::level::info, "hello" );

	REQUIRE( line.find( "\"logger\":\"test\",\"msg\":\"hello\"" ) != std::string::npos );
}

TEST_CASE( "JSON formatter reports the level name", "[logger]" )
{
	REQUIRE( format( spdlog::level::err, "x" ).find( "\"level\":\"error\"" ) != std::string::npos );
	REQUIRE( format( spdlog::level::debug, "x" ).find( "\"level\":\"debug\"" ) != std::string::npos );
}

namespace {

// same log_msg through the console pattern (with %* fields flag) and
// through spdlog's default pattern
std::pair<std::string, std::string> formatConsole( const std::string & payload )
{
	spdlog::details::log_msg	msg( "test", spdlog::level::info, payload );
	spdlog::pattern_formatter	withFields;
	spdlog::pattern_formatter	byDefault;
	spdlog::memory_buf_t		fieldsDest;
	spdlog::memory_buf_t		defaultDest;

	withFields.add_flag<drea::core::integrations::logs::text_fields_flag>( '*' ).set_pattern( "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %*%v" );
	withFields.format( msg, fieldsDest );
	byDefault.format( msg, defaultDest );
	return { std::string( fieldsDest.data(), fieldsDest.size() ), std::string( defaultDest.data(), defaultDest.size() ) };
}

}

TEST_CASE( "console pattern renders fields as [key:value] blocks", "[logger]" )
{
	drea::log::mdc::clear();
	drea::log::mdc::put( "session", "abc-123" );
	drea::log::mdc::put( "user", "dave" );

	const auto [line, unused] = formatConsole( "hello" );

	drea::log::mdc::clear();
	REQUIRE( line.find( "[session:abc-123][user:dave] hello" ) != std::string::npos );
}

TEST_CASE( "console pattern with empty MDC matches spdlog's default output", "[logger]" )
{
	drea::log::mdc::clear();

	const auto [line, defaultLine] = formatConsole( "hello" );

	REQUIRE( line == defaultLine );
}

TEST_CASE( "Logger wrapper emits per-call fields in the JSON output", "[logger]" )
{
	CapturedLogger cap;

	drea::log::mdc::clear();
	cap.logger.info( { "session", "abc-123" }, "hello {}", 42 );

	REQUIRE( cap.text().find( "\"session\":\"abc-123\",\"msg\":\"hello 42\"" ) != std::string::npos );
	REQUIRE( drea::log::mdc::get_context().empty() );
}

TEST_CASE( "Logger wrapper accepts multiple fields", "[logger]" )
{
	CapturedLogger cap;

	drea::log::mdc::clear();
	cap.logger.warn( { { "session", "abc-123" }, { "user", "dave" } }, "hello" );

	REQUIRE( cap.text().find( "\"session\":\"abc-123\",\"user\":\"dave\",\"msg\":\"hello\"" ) != std::string::npos );
	REQUIRE( drea::log::mdc::get_context().empty() );
}

TEST_CASE( "Logger wrapper logs at a runtime-chosen level with fields", "[logger]" )
{
	CapturedLogger cap;

	drea::log::mdc::clear();
	cap.logger.log( spdlog::level::err, { "session", "abc-123" }, "dynamic {}", 1 );
	cap.logger.log( spdlog::level::warn, { { "session", "abc-123" }, { "user", "dave" } }, "multi" );
	cap.logger.log( spdlog::level::info, "plain" );

	REQUIRE( cap.text().find( "\"level\":\"error\"" ) != std::string::npos );
	REQUIRE( cap.text().find( "\"session\":\"abc-123\",\"msg\":\"dynamic 1\"" ) != std::string::npos );
	REQUIRE( cap.text().find( "\"level\":\"warning\"" ) != std::string::npos );
	REQUIRE( cap.text().find( "\"session\":\"abc-123\",\"user\":\"dave\",\"msg\":\"multi\"" ) != std::string::npos );
	REQUIRE( cap.text().find( "\"msg\":\"plain\"" ) != std::string::npos );
	REQUIRE( drea::log::mdc::get_context().empty() );
}

TEST_CASE( "Logger wrapper should_log follows the underlying level", "[logger]" )
{
	CapturedLogger cap;

	cap.raw.set_level( spdlog::level::warn );

	REQUIRE_FALSE( cap.logger.should_log( spdlog::level::info ) );
	REQUIRE( cap.logger.should_log( spdlog::level::err ) );
}

TEST_CASE( "Logger wrapper skips fields with an empty value", "[logger]" )
{
	CapturedLogger cap;

	drea::log::mdc::clear();
	cap.logger.info( { "session", "" }, "hello" );

	REQUIRE( cap.text().find( "\"session\"" ) == std::string::npos );
	REQUIRE( cap.text().find( "\"msg\":\"hello\"" ) != std::string::npos );
	REQUIRE( drea::log::mdc::get_context().empty() );
}

TEST_CASE( "Logger wrapper leaves MDC untouched on a disabled level", "[logger]" )
{
	CapturedLogger cap;

	cap.raw.set_level( spdlog::level::warn );
	drea::log::mdc::clear();
	drea::log::mdc::put( "outer", "kept" );

	cap.logger.debug( { "session", "abc-123" }, "hidden" );

	REQUIRE( cap.text().empty() );
	REQUIRE( drea::log::mdc::get( "outer" ) == "kept" );
	REQUIRE( drea::log::mdc::get_context().size() == 1 );
	drea::log::mdc::clear();
}

TEST_CASE( "Logger wrapper overwrites a colliding MDC key and removes it after", "[logger]" )
{
	CapturedLogger cap;

	drea::log::mdc::clear();
	drea::log::mdc::put( "session", "outer" );

	cap.logger.info( { "session", "inner" }, "hello" );

	REQUIRE( cap.text().find( "\"session\":\"inner\"" ) != std::string::npos );
	// removed, not restored to "outer"
	REQUIRE( drea::log::mdc::get( "session" ).empty() );
	drea::log::mdc::clear();
}

TEST_CASE( "Field composes with redacted()", "[logger]" )
{
	CapturedLogger cap;

	drea::log::detail::setRedactionEnabled( true );
	drea::log::mdc::clear();
	cap.logger.info( { "email", drea::log::redacted( "dave@example.com" ) }, "hello" );

	REQUIRE( cap.text().find( "\"email\":\"[redacted]\"" ) != std::string::npos );
	REQUIRE( cap.text().find( "dave@example.com" ) == std::string::npos );
}

TEST_CASE( "Logger wrapper passthrough matches the raw logger output", "[logger]" )
{
	CapturedLogger viaWrapper;
	CapturedLogger viaRaw;

	drea::log::mdc::clear();
	viaWrapper.logger.info( "hello {}", 42 );
	viaRaw.raw.info( "hello {}", 42 );

	const auto tail = []( const std::string & line ){ return line.substr( line.find( "\",\"level\"" ) ); };

	REQUIRE( tail( viaWrapper.text() ) == tail( viaRaw.text() ) );
}

TEST_CASE( "App::logger retargets from the default logger to the configured one", "[logger]" )
{
	AppFixture fx;

	REQUIRE( &fx.app.logger().raw() == spdlog::default_logger().get() );
	REQUIRE( &fx.app.logger() == &fx.app.logger() );

	fx.app.parse( "app: logger-retarget-test\n" );

	REQUIRE( &fx.app.logger().raw() != spdlog::default_logger().get() );
	REQUIRE( fx.app.logger().raw().name() == fx.app.name() );
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
	drea::log::Logger wrapped( *logger );
	wrapped.info( "json goes to the file" );
	wrapped.flush();

	std::ifstream	in( logFile );
	std::string		line;
	REQUIRE( std::getline( in, line ) );
	REQUIRE( line.front() == '{' );
	REQUIRE( line.find( "\"msg\":\"json goes to the file\"" ) != std::string::npos );
	REQUIRE( line.back() == '}' );

	in.close();
	std::filesystem::remove( logFile );
}

TEST_CASE( "drea::log::mdc stores per-thread key/values", "[logger][mdc]" )
{
	drea::log::mdc::clear();
	drea::log::mdc::put( "session", "abc" );

	REQUIRE( drea::log::mdc::get( "session" ) == "abc" );
	REQUIRE( drea::log::mdc::get( "missing" ).empty() );
	REQUIRE( drea::log::mdc::get_context().size() == 1 );

	drea::log::mdc::put( "session", "def" );
	REQUIRE( drea::log::mdc::get( "session" ) == "def" );

	drea::log::mdc::remove( "session" );
	REQUIRE( drea::log::mdc::get_context().empty() );

	// another thread sees its own, empty context
	std::string other = "sentinel";
	std::thread( [&other]{ other = drea::log::mdc::get( "session" ); } ).join();
	REQUIRE( other.empty() );
}
