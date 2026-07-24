#pragma once

#include <spdlog/pattern_formatter.h>

#include <drea/log/Mdc.h>

#include <memory>

namespace drea::core::integrations::logs {

// Custom pattern flag for the console sink: prints the thread's MDC entries
// as "[key:value]" blocks followed by one space, or nothing when the map is
// empty — so lines without fields stay byte-identical to spdlog's default
// pattern:
//
// [2026-07-14 10:22:31.045] [myapp] [info] [session:abc-123] listening on :8080
//
// Registered as %* in Config::setupLogger. Same synchronous-logger-only
// caveat as the JSON formatter.
class text_fields_flag : public spdlog::custom_flag_formatter
{
public:
	void format( const spdlog::details::log_msg &, const std::tm &, spdlog::memory_buf_t & dest ) override
	{
		const auto & context = drea::log::mdc::get_context();

		if( context.empty() ){
			return;
		}
		for( const auto & [key, value] : context ){
			dest.push_back( '[' );
			dest.append( key.data(), key.data() + key.size() );
			dest.push_back( ':' );
			dest.append( value.data(), value.data() + value.size() );
			dest.push_back( ']' );
		}
		dest.push_back( ' ' );
	}

	[[nodiscard]] std::unique_ptr<custom_flag_formatter> clone() const override
	{
		return std::make_unique<text_fields_flag>();
	}
};

}
