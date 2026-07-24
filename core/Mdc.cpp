#include <drea/log/Mdc.h>

namespace {

// one map per thread, owned by the library so every module of a process
// (the app, drea itself, plugins) sees the same instance
drea::log::mdc::mdc_map_t & context()
{
	static thread_local drea::log::mdc::mdc_map_t instance;
	return instance;
}

}

void drea::log::mdc::put( const std::string & key, const std::string & value )
{
	::context()[ key ] = value;
}

std::string drea::log::mdc::get( const std::string & key )
{
	if( const auto it = ::context().find( key ); it != ::context().end() ){
		return it->second;
	}
	return {};
}

void drea::log::mdc::remove( const std::string & key )
{
	::context().erase( key );
}

void drea::log::mdc::clear()
{
	::context().clear();
}

drea::log::mdc::mdc_map_t & drea::log::mdc::get_context()
{
	return ::context();
}
