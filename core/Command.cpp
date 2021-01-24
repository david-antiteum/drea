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

std::string drea::core::Command::nameOfParamsForHelp() const
{
	std::string		res;
	auto names = utilities::string::split( mParamName, " " );
	for( auto name: names ){
		if( !res.empty() ){
			res += " ";
		}
		res += fmt::format( "[{}]", name );
	}
	return res;
}
