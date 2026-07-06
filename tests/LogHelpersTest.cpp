#include <catch2/catch_test_macros.hpp>

#include <drea/log/CorrelationId.h>
#include <drea/log/Redacted.h>
#include <drea/core/ExitCode.h>

#include <spdlog/fmt/fmt.h>

#include <string>

namespace {

// restore the global redaction flag on scope exit so tests do not leak state
struct RedactionGuard {
	~RedactionGuard() { drea::log::detail::setRedactionEnabled( true ); }
};

}

TEST_CASE( "valid correlation ids pass through", "[correlation-id]" )
{
	CHECK( drea::log::sanitizeCorrelationId( "abc123" ) == "abc123" );
	CHECK( drea::log::sanitizeCorrelationId( "550e8400-e29b-41d4-a716-446655440000" )
		== "550e8400-e29b-41d4-a716-446655440000" );
	CHECK( drea::log::sanitizeCorrelationId( "session.1_A-b" ) == "session.1_A-b" );
}

TEST_CASE( "empty and oversized ids are rejected", "[correlation-id]" )
{
	CHECK( drea::log::sanitizeCorrelationId( "" ).empty() );
	const std::string oversized( drea::log::kMaxCorrelationIdLength + 1, 'a' );
	CHECK( drea::log::sanitizeCorrelationId( oversized ).empty() );
	const std::string atLimit( drea::log::kMaxCorrelationIdLength, 'a' );
	CHECK( drea::log::sanitizeCorrelationId( atLimit ) == atLimit );
}

TEST_CASE( "log-injection characters are rejected", "[correlation-id]" )
{
	CHECK( drea::log::sanitizeCorrelationId( "abc def" ).empty() );  // space → field spoofing
	CHECK( drea::log::sanitizeCorrelationId( "abc\ndef" ).empty() ); // newline → line injection
	CHECK( drea::log::sanitizeCorrelationId( "abc=def" ).empty() );  // '=' → key=value spoofing
	CHECK( drea::log::sanitizeCorrelationId( "abc\"def" ).empty() ); // quote → JSON escape games
	CHECK( drea::log::sanitizeCorrelationId( "abc\x1b[0m" ).empty() ); // ANSI escape
}

TEST_CASE( "redacted() hides values by default", "[redacted]" )
{
	RedactionGuard guard;
	const std::string email = "user@example.com";

	CHECK( fmt::format( "email {}", drea::log::redacted( email ) ) == "email [redacted]" );
	CHECK( fmt::format( "id {}", drea::log::redacted( 42 ) ) == "id [redacted]" );
	CHECK( fmt::format( "name {}", drea::log::redacted( "literal" ) ) == "name [redacted]" );
}

TEST_CASE( "redacted() prints the value when redaction is off", "[redacted]" )
{
	RedactionGuard guard;
	drea::log::detail::setRedactionEnabled( false );
	const std::string email = "user@example.com";

	CHECK( fmt::format( "email {}", drea::log::redacted( email ) ) == "email user@example.com" );
	CHECK( fmt::format( "id {}", drea::log::redacted( 42 ) ) == "id 42" );
	CHECK( fmt::format( "pi {:.2f}", drea::log::redacted( 3.14159 ) ) == "pi 3.14" );
}

TEST_CASE( "exit codes carry the sysexits values", "[exit-code]" )
{
	CHECK( drea::core::toInt( drea::core::ExitCode::Ok ) == 0 );
	CHECK( drea::core::toInt( drea::core::ExitCode::DependencyError ) == 69 );
	CHECK( drea::core::toInt( drea::core::ExitCode::ConfigError ) == 78 );
}
