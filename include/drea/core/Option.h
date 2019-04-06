#pragma once

#include <string>
#include <vector>
#include <typeindex>
#include <variant>

#include "Export.h"

namespace drea { namespace core {

using OptionValue = std::variant<std::monostate,bool,int,double,std::string>;

struct DREA_CORE_API Option
{
	std::string 				mName;
	std::string					mParamName;
	std::string					mDescription;
	std::vector<OptionValue>	mValues;
	std::type_index				mType = typeid( std::string );

	std::string toString( const OptionValue & val ) const;
	OptionValue fromString( const std::string & val ) const;
};

}}
