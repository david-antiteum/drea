#pragma once

#include <algorithm>
#include <string>
#include <string_view>

namespace drea::log {

// Client-supplied correlation values (request ids, session ids) end up in
// log lines. Clamp charset and length so a hostile client cannot inject log
// fields (key=value spoofing, newlines) or oversized values.
// Header-only + pure std so it is unit-testable anywhere.

inline constexpr std::size_t kMaxCorrelationIdLength = 64;

// Accept only [0-9A-Za-z._-], non-empty, at most kMaxCorrelationIdLength.
[[nodiscard]] inline bool isValidCorrelationId( std::string_view value ) noexcept
{
	if( value.empty() || value.size() > kMaxCorrelationIdLength ){
		return false;
	}
	return std::all_of( value.begin(), value.end(), []( const char c ){
		return ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'Z' )
			|| ( c >= 'a' && c <= 'z' ) || c == '.' || c == '_' || c == '-';
	} );
}

// Pass a valid correlation id through; anything else becomes empty (caller
// then treats it as absent — e.g. mints a fresh request id).
[[nodiscard]] inline std::string sanitizeCorrelationId( std::string_view value )
{
	return isValidCorrelationId( value ) ? std::string( value ) : std::string{};
}

}
