#pragma once

#include <map>
#include <string>

#include <drea/core/Export.h>

namespace drea::log {

/*! Thread-local mapped diagnostic context: string key/value pairs that the
	drea formatters merge into every log record formatted on this thread —
	as extra top-level attributes in the JSON file log and as `[key:value]`
	blocks on the console.

	Two ways to fill it:
	- per call, via drea::log::Logger fields (put, log, remove — handled
	  internally);
	- per scope, directly: put request-scoped context (session, correlation
	  id) once and every log line of the request carries it.

	\code
	drea::log::mdc::put( "session", sessionId );
	// ... handle the request: every log call includes [session:...]
	drea::log::mdc::clear();
	\endcode

	Same shape as spdlog::mdc, but self-contained: drea does not require
	spdlog 1.15. The storage lives in the drea library (one instance per
	thread, shared across modules), and only synchronous loggers see it —
	formatting must run on the thread that populated the context.
*/
class DREA_CORE_API mdc
{
public:
	using mdc_map_t = std::map<std::string, std::string>;

	static void put( const std::string & key, const std::string & value );
	[[nodiscard]] static std::string get( const std::string & key );
	static void remove( const std::string & key );
	static void clear();
	[[nodiscard]] static mdc_map_t & get_context();
};

}
