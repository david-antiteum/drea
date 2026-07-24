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
	int								mMinParams = -1;
	bool							mHidden = false;
	std::vector<std::string>		mGroups;
	std::vector<std::string>		mExamples;	//!< worked invocations, e.g. "myapp copy src.txt dst.txt"; shown in describe
	bool							mDeprecated = false;	//!< kept working but discouraged; flagged in help and describe
	bool							mPredefined = false;	//!< added by Commander::addDefaults; shown under "Common commands" in help
	static const int				mUnlimitedParams = 0xfffffffa;

	[[nodiscard]] int numberOfParams() const;
	[[nodiscard]] int minParams() const;
	[[nodiscard]] int maxParams() const;
	[[nodiscard]] std::string nameOfParamsForHelp() const;

	//! Set automatically using parent information
	std::vector<std::string>		mSubcommand;
};

}
