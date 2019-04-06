#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "Export.h"
#include "Command.h"

namespace drea { namespace core {

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

	void showHelp( const std::string & command ) const;

	void reportNoCommand( const std::string & command ) const;

	void commands( std::function<void(const Command&)> f ) const;

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}}
