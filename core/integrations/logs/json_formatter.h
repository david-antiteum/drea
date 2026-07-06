#pragma once

#include <spdlog/formatter.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/details/os.h>
#include <spdlog/fmt/fmt.h>

#include <chrono>
#include <ctime>
#include <memory>
#include <string_view>

namespace drea::core::integrations::logs {

// Formats each record as one JSON object per line, for file sinks:
//
// {"timestamp":"2026-07-06T12:34:56.789+02:00","level":"info","logger":"myapp","msg":"hello"}
//
// A real formatter (not a set_pattern string) so that the message payload can
// be JSON-escaped: embedded quotes, backslashes and control characters do not
// corrupt the stream.
class json_lines_formatter : public spdlog::formatter
{
public:
	void format( const spdlog::details::log_msg & msg, spdlog::memory_buf_t & dest ) override
	{
		appendLiteral( dest, "{\"timestamp\":\"" );
		appendTimestamp( dest, msg.time );
		appendLiteral( dest, "\",\"level\":\"" );
		appendEscaped( dest, spdlog::level::to_string_view( msg.level ) );
		appendLiteral( dest, "\",\"logger\":\"" );
		appendEscaped( dest, msg.logger_name );
		appendLiteral( dest, "\",\"msg\":\"" );
		appendEscaped( dest, msg.payload );
		appendLiteral( dest, "\"}" );
		appendLiteral( dest, spdlog::details::os::default_eol );
	}

	[[nodiscard]] std::unique_ptr<spdlog::formatter> clone() const override
	{
		return std::make_unique<json_lines_formatter>();
	}

private:
	static void appendLiteral( spdlog::memory_buf_t & dest, std::string_view text )
	{
		dest.append( text.data(), text.data() + text.size() );
	}

	// ISO 8601 with milliseconds and local timezone offset
	static void appendTimestamp( spdlog::memory_buf_t & dest, spdlog::log_clock::time_point time )
	{
		const std::time_t	tt = spdlog::log_clock::to_time_t( time );
		const std::tm		tm = spdlog::details::os::localtime( tt );
		const auto			secs = std::chrono::duration_cast<std::chrono::seconds>( time.time_since_epoch() );
		const auto			millis = std::chrono::duration_cast<std::chrono::milliseconds>( time.time_since_epoch() ) - secs;
		char				date[32] = {};
		char				zone[8] = {};

		std::strftime( date, sizeof( date ), "%Y-%m-%dT%H:%M:%S", &tm );
		std::strftime( zone, sizeof( zone ), "%z", &tm );
		fmt::format_to( std::back_inserter( dest ), "{}.{:03}", date, millis.count() );
		if( std::string_view offset{ zone }; offset.size() == 5 ){
			// "+0200" -> "+02:00"
			fmt::format_to( std::back_inserter( dest ), "{}:{}", offset.substr( 0, 3 ), offset.substr( 3 ) );
		}else{
			appendLiteral( dest, offset );
		}
	}

	static void appendEscaped( spdlog::memory_buf_t & dest, spdlog::string_view_t text )
	{
		for( const char c: std::string_view( text.data(), text.size() ) ){
			switch( c ){
				case '"':
					appendLiteral( dest, "\\\"" );
				break;

				case '\\':
					appendLiteral( dest, "\\\\" );
				break;

				case '\n':
					appendLiteral( dest, "\\n" );
				break;

				case '\r':
					appendLiteral( dest, "\\r" );
				break;

				case '\t':
					appendLiteral( dest, "\\t" );
				break;

				case '\b':
					appendLiteral( dest, "\\b" );
				break;

				case '\f':
					appendLiteral( dest, "\\f" );
				break;

				default:
					if( static_cast<unsigned char>( c ) < 0x20 ){
						fmt::format_to( std::back_inserter( dest ), "\\u{:04x}", static_cast<unsigned char>( c ) );
					}else{
						dest.push_back( c );
					}
			}
		}
	}
};

}
