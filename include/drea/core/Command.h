#pragma once

#include <string>
#include <vector>

#include "Export.h"

namespace drea { namespace core {

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
		
	int numberOfParams() const
	{
		if( mParamName.empty() ){
			return 0;
		}else{
			return mNbParams;
		}
	}

	//! Set automatically using parent information
	std::vector<std::string>		mSubcommand;
};

}}
