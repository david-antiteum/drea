#pragma once

#include <spdlog/spdlog.h>

#include <drea/log/Mdc.h>

#include <initializer_list>
#include <string>
#include <utility>

namespace drea::log {

/*! One structured field attached to a single log call. The JSON file sink
	emits it as a top-level attribute, the console as a `[key:value]` block:

	\code
	logger.info( { "session", sessionId }, "License installed for {}", user );
	// file:    {...,"session":"abc-123","msg":"License installed for dave"}
	// console: [...] [info] [session:abc-123] License installed for dave
	\endcode

	The value can be anything fmt-formattable and composes with
	drea::log::redacted(): `Field{ "email", drea::log::redacted( email ) }`.
	Values are formatted when the Field is constructed, so redaction resolves
	the same way as message arguments.
*/
struct Field
{
	std::string key;
	std::string value;

	template<typename T>
	Field( std::string k, const T & v )
		: key( std::move( k ) ), value( fmt::format( "{}", v ) )
	{
	}
};

/*! Thin wrapper over spdlog::logger that adds per-call structured fields,
	using drea::log::mdc as a hidden transport: put the fields, log, remove
	them. Safe because drea loggers are synchronous — formatting runs on the
	calling thread inside log() (never wrap an spdlog::async_logger: its
	backend thread sees an empty MDC).

	Fields with an empty value are skipped. A per-call key colliding with an
	entry already in the thread's MDC is overwritten for the call and removed
	after — not restored.
*/
class Logger
{
public:
	explicit Logger( spdlog::logger & logger )
		: mLogger( &logger )
	{
	}

	//! retarget the wrapper, e.g. once Config::setupLogger has built the real logger
	void reset( spdlog::logger & logger ) noexcept
	{
		mLogger = &logger;
	}

	// passthrough — call sites without fields behave exactly like spdlog
	template<typename... Args>
	void trace( fmt::format_string<Args...> fmt, Args &&... args )
	{
		mLogger->trace( fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void debug( fmt::format_string<Args...> fmt, Args &&... args )
	{
		mLogger->debug( fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void info( fmt::format_string<Args...> fmt, Args &&... args )
	{
		mLogger->info( fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void warn( fmt::format_string<Args...> fmt, Args &&... args )
	{
		mLogger->warn( fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void error( fmt::format_string<Args...> fmt, Args &&... args )
	{
		mLogger->error( fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void critical( fmt::format_string<Args...> fmt, Args &&... args )
	{
		mLogger->critical( fmt, std::forward<Args>( args )... );
	}

	// runtime-chosen level: log( level, "...", ... )
	template<typename... Args>
	void log( spdlog::level::level_enum lvl, fmt::format_string<Args...> fmt, Args &&... args )
	{
		mLogger->log( lvl, fmt, std::forward<Args>( args )... );
	}

	// single field: info( { "session", id }, "...", ... )
	template<typename... Args>
	void trace( Field field, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::trace, &field, &field + 1, fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void debug( Field field, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::debug, &field, &field + 1, fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void info( Field field, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::info, &field, &field + 1, fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void warn( Field field, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::warn, &field, &field + 1, fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void error( Field field, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::err, &field, &field + 1, fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void critical( Field field, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::critical, &field, &field + 1, fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void log( spdlog::level::level_enum lvl, Field field, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( lvl, &field, &field + 1, fmt, std::forward<Args>( args )... );
	}

	// multiple: info( { { "session", id }, { "user", u } }, "...", ... )
	template<typename... Args>
	void trace( std::initializer_list<Field> fields, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::trace, fields.begin(), fields.end(), fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void debug( std::initializer_list<Field> fields, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::debug, fields.begin(), fields.end(), fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void info( std::initializer_list<Field> fields, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::info, fields.begin(), fields.end(), fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void warn( std::initializer_list<Field> fields, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::warn, fields.begin(), fields.end(), fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void error( std::initializer_list<Field> fields, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::err, fields.begin(), fields.end(), fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void critical( std::initializer_list<Field> fields, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( spdlog::level::critical, fields.begin(), fields.end(), fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void log( spdlog::level::level_enum lvl, std::initializer_list<Field> fields, fmt::format_string<Args...> fmt, Args &&... args )
	{
		logFields( lvl, fields.begin(), fields.end(), fmt, std::forward<Args>( args )... );
	}

	//! guard expensive log-argument computation
	[[nodiscard]] bool should_log( spdlog::level::level_enum lvl ) const
	{
		return mLogger->should_log( lvl );
	}

	//! flush the underlying logger's sinks
	void flush()
	{
		mLogger->flush();
	}

	//! escape hatch for sinks, level, ...
	spdlog::logger & raw() noexcept
	{
		return *mLogger;
	}

private:
	// removes the keys even if formatting throws
	struct MdcGuard
	{
		const Field * begin;
		const Field * end;

		~MdcGuard()
		{
			for( const Field * it = begin; it != end; ++it ){
				if( !it->value.empty() ){
					drea::log::mdc::remove( it->key );
				}
			}
		}
	};

	template<typename... Args>
	void logFields( spdlog::level::level_enum lvl, const Field * begin, const Field * end, fmt::format_string<Args...> fmt, Args &&... args )
	{
		if( !mLogger->should_log( lvl ) ){
			return;
		}

		MdcGuard guard{ begin, end };

		for( const Field * it = begin; it != end; ++it ){
			// empty value = field skipped, like the old "empty context
			// prepends nothing" convention
			if( !it->value.empty() ){
				drea::log::mdc::put( it->key, it->value );
			}
		}
		mLogger->log( lvl, fmt, std::forward<Args>( args )... );
	}

	spdlog::logger * mLogger;
};

}
