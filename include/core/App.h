#pragma once

#include <string>
#include <memory>

#include "Export.h"

namespace spdlog {
	class logger;
}

namespace drea { namespace core {

class Config;
class Commander;

class DREA_CORE_API App
{
public:
    explicit App();
	~App();

	static App & instance();

	void setName( const std::string & value );
    void setDescription( const std::string & value );
    void setVersion( const std::string & value );

    void parse( int argc, char * argv[] );

    Config & config() const;
    Commander & commander() const;

	std::shared_ptr<spdlog::logger> logger() const;

	[[noreturn]] void showVersion();
	[[noreturn]] void showHelp();
	
private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}}
