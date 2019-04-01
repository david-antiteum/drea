#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>

#include "Export.h"
#include "Option.h"

namespace drea { namespace core {

class DREA_CORE_API Config
{
public:
    explicit Config();
	~Config();

	std::vector<std::string> configure( int argc, char * argv[] );

	Config & addDefaults();
    Config & add( Option option );

	std::optional<Option*> find( const std::string & flag ) const;

	bool contains( const std::string & flag ) const;

	template<typename T>
	T value( const std::string & flag ) const
	{
		std::optional<Option*> 	option = find( flag );

		if( option && !option.value()->mValues.empty() ){
			return std::get<T>( option.value()->mValues.front() );
		}
		return T{};
	}

	void showHelp( int offset );

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}}
