#pragma once

#include <string>
#include <vector>

#include "Export.h"

namespace drea::core {

struct DREA_CORE_API Command
{	
	std::string 					mName;
	std::string						mParamName;
	std::string						mDescription;
	std::vector<std::string>		mLocalParameters;
	std::vector<std::string>		mGlobalParameters;
	std::string						mParentCommand;
	int								mNbParams = 1;
	static const int				mUnlimitedParams = 0xfffffffa;
		
	[[nodiscard]] int numberOfParams() const;
	[[nodiscard]] std::string nameOfParamsForHelp() const;

	//! Set automatically using parent information
	std::vector<std::string>		mSubcommand;
};

}
