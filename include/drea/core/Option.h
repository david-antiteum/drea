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
	int							mNbItems = 1;
	std::string					mShortVersion; 					
	static const int			mUnlimitedParams = 0xfffffffa;

	int numberOfParams() const
	{
		if( mParamName.empty() ){
			return 0;
		}else{
			return mNbItems;
		}
	}

	std::string toString( const OptionValue & val ) const;
	OptionValue fromString( const std::string & val ) const;
};

}}
