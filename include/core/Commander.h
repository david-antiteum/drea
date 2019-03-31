#pragma once

#include <string>
#include <functional>
#include <memory>

#include "Export.h"

namespace drea { namespace core {

struct DREA_CORE_API Command
{	
    std::string 					mName;
	std::string						mDescription;
	std::vector<std::string>		mLocalParameters;
	std::vector<std::string>		mGlobalParameters;
};

class DREA_CORE_API Commander
{
public:
    explicit Commander();
	~Commander();

	void configure( const std::vector<std::string> & args );

	Commander & addDefaults();

    Commander & add( Command cmd );

    void run( std::function<void( std::string )> f );

	std::vector<std::string> arguments();

	void showHelp( const std::string & command );

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}}
