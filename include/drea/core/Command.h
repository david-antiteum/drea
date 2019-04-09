#pragma once

#include <string>
#include <vector>

#include "Export.h"

namespace drea { namespace core {

struct DREA_CORE_API Command
{	
	std::string 					mName;
	std::string						mDescription;
	std::vector<std::string>		mLocalParameters;
	std::vector<std::string>		mGlobalParameters;
	std::string						mParentCommand;
	std::vector<std::string>		mSubcommand;
};

}}
