#pragma once

#include <optional>
#include <string>
#include <vector>
#include <typeindex>
#include <variant>

#include "Export.h"

namespace drea::core {

using OptionValue = std::variant<std::monostate,bool,int,double,std::string>;

struct DREA_CORE_API Option
{
	enum class Scope {
		Both,
		File,
		Line,
		None
	};

	std::string 				mName;
	std::string					mParamName;
	std::string					mDescription;
	std::vector<OptionValue>	mValues = {};
	std::type_index				mType = typeid( std::string );
	Scope						mScope = Scope::Both;
	int							mNbParams = 1;
	std::string					mShortVersion = "";
	bool						mSensitive = false;
	bool						mRequired = false;
	std::optional<double>		mMin = std::nullopt;
	std::optional<double>		mMax = std::nullopt;
	std::vector<std::string>	mChoices = {};	//!< closed set of legal values (compared against Option::toString); empty = unrestricted
	bool						mNegatable = true;	//!< bool options accept --no-<name>. Set false for an action (--help, --version, --validate): denying it has no meaning, so --no-<name> is refused instead of silently triggering it
	bool						mDeprecated = false;	//!< kept working but discouraged; flagged in help and describe
	bool						mPredefined = false;	//!< added by Config::addDefaults; shown under "Common options" in help
	static const int			mUnlimitedParams = 0xfffffffa;

	[[nodiscard]] int numberOfParams() const
	{
		if( mParamName.empty() ){
			return 0;
		}else{
			return mNbParams;
		}
	}

	//! Whether the option takes an open ended list of values (params: unlimited)
	[[nodiscard]] bool unlimitedParams() const
	{
		return !mParamName.empty() && mNbParams == mUnlimitedParams;
	}

	/*! Whether the option takes at least one value per use, unlimited included.

		Use this rather than comparing numberOfParams(), which returns the
		mUnlimitedParams sentinel for an open ended option: the sentinel is
		negative as an int, so "numberOfParams() > 0" reads it as a flag.
	*/
	[[nodiscard]] bool takesValues() const
	{
		return unlimitedParams() || numberOfParams() > 0;
	}

	[[nodiscard]] std::string toString( const OptionValue & val ) const;
	[[nodiscard]] OptionValue fromString( const std::string & val ) const;

	/*! The declared type as text: "bool", "int", "double" or "string".
		The same closed set that the describe builtin emits.
	*/
	[[nodiscard]] std::string typeName() const;

	/*! The scope as text: "both", "command-line", "config-file" or "none".
		The same closed set that the describe builtin emits.
	*/
	[[nodiscard]] std::string scopeName() const;

	[[nodiscard]] bool helpInLine() const
	{
		return mScope == Scope::Both || mScope == Scope::Line;
	}

	[[nodiscard]] bool helpInFileOnly() const
	{
		return mScope == Scope::File;
	}

};

}
