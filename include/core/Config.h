#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>

#include "Export.h"

namespace drea { namespace core {

struct DREA_CORE_API Option
{
	std::string 				mName;
	std::string					mParamName;
	std::string					mDescription;
	std::vector<std::string>	mValues;
};

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
	std::string value( const std::string & flag ) const;

	void showHelp( int offset );

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}}
