#include <algorithm>

#include "Command.h"
#include "utilities/string.h"

#include <spdlog/fmt/fmt.h>

int drea::core::Command::numberOfParams() const
{
	if( mParamName.empty() ){
		return 0;
	}else{
		return mNbParams;
	}
}

int drea::core::Command::minParams() const
{
	if( mMinParams >= 0 ){
		return mMinParams;
	}
	return numberOfParams();
}

int drea::core::Command::maxParams() const
{
	return numberOfParams();
}

std::string drea::core::Command::nameOfParamsForHelp() const
{
	std::string		res;
	auto names = utilities::string::split( mParamName, " " );
	const bool unlimited = mNbParams == mUnlimitedParams;
	// Names before the minimum are required (<name>), the rest are optional
	// ([name]). With params: unlimited the count is open ended, so a name is
	// required only when min-params says so.
	const int minP = unlimited ? std::max( 0, mMinParams ) : minParams();

	for( size_t i = 0; i < names.size(); ++i ){
		if( !res.empty() ){
			res += " ";
		}
		if( static_cast<int>( i ) < minP ){
			res += fmt::format( "<{}>", names[i] );
		}else{
			res += fmt::format( "[{}]", names[i] );
		}
	}
	if( unlimited && !res.empty() ){
		res += "...";
	}
	return res;
}
