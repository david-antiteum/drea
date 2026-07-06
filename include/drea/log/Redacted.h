#pragma once

#include <spdlog/fmt/fmt.h>

#include <atomic>
#include <string_view>
#include <type_traits>

namespace drea::log {

/*! Runtime counterpart of the `sensitive: true` option flag. Wrap any value
	that must not reach production logs:

	\code
	app.logger().debug( "user email {}", drea::log::redacted( email ) );
	\endcode

	Prints `[redacted]` when redaction is on (the default), the value
	otherwise. Driven by the predefined `log-redact` option, read once by
	App::parse and frozen; dev turns it off with `--no-log-redact`.

	The wrapper holds a view of the value — use it only inside the log
	statement, do not store it.
*/

namespace detail {

inline std::atomic<bool> & redactionFlag() noexcept
{
	static std::atomic<bool> flag{ true };
	return flag;
}

// Called once by App::parse from the log-redact option. Not meant as a
// runtime toggle: config is immutable after startup.
inline void setRedactionEnabled( bool on ) noexcept
{
	redactionFlag().store( on, std::memory_order_relaxed );
}

// String-likes collapse to a view, arithmetic types are copied, anything
// else is held by reference. Zero allocation in all cases.
template <typename T>
using StoredType = std::conditional_t<
	std::is_convertible_v<const T &, std::string_view>,
	std::string_view,
	std::conditional_t<std::is_arithmetic_v<T>, T, const T &>>;

}

[[nodiscard]] inline bool redactionEnabled() noexcept
{
	return detail::redactionFlag().load( std::memory_order_relaxed );
}

template <typename T>
struct Redacted
{
	detail::StoredType<T> mValue;
};

template <typename T>
[[nodiscard]] Redacted<T> redacted( const T & value ) noexcept
{
	return Redacted<T>{ value };
}

}

template <typename T, typename Char>
struct fmt::formatter<drea::log::Redacted<T>, Char>
	: fmt::formatter<std::remove_cv_t<std::remove_reference_t<drea::log::detail::StoredType<T>>>, Char>
{
	template <typename FormatContext>
	auto format( const drea::log::Redacted<T> & wrapper, FormatContext & ctx ) const
	{
		if( drea::log::redactionEnabled() ){
			return fmt::format_to( ctx.out(), "[redacted]" );
		}
		using Base = fmt::formatter<std::remove_cv_t<std::remove_reference_t<drea::log::detail::StoredType<T>>>, Char>;
		return Base::format( wrapper.mValue, ctx );
	}
};
