#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>
#include <functional>
#include <object_ptr/object_ptr.hpp>

#include "Export.h"
#include "Option.h"

namespace drea { namespace core {

class DREA_CORE_API Config
{
public:
	explicit Config();
	~Config();

	std::vector<std::string> configure( int argc, char * argv[] );

	void addDefaults();
	void add( Option option );

	void options( std::function<void(const Option&)> f ) const;

	void setEnvPrefix( const std::string & value );
	void addRemoteProvider( const std::string & provider, const std::string & host, const std::string & key );

	jss::object_ptr<Option> find( const std::string & flag ) const;

	bool contains( const std::string & flag ) const;

	void set( const std::string & flag, const std::string & val );

	template<typename T>
	T get( const std::string & flag ) const
	{
		auto option = find( flag );

		if( option && !option->mValues.empty() ){
			return std::get<T>( option->mValues.front() );
		}
		return T{};
	}

	bool empty() const;

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}}
