#pragma once

#include <string>
#include <memory>
#include <vector>

#include "Export.h"

namespace spdlog {
	class logger;
}

namespace drea { namespace core {

class Config;
class Commander;

/*! Our application (or service). It has support for:
	- a command system: App::commander
	- a configuration system: App::config
	- a logging system: App::logger
*/
class DREA_CORE_API App
{
public:
	explicit App( int argc, char * argv[] );
	~App();

	/*! Access to the single instance of the app
	*/
	static App & instance();

	const std::string & name() const;
	const std::string & description() const;
	const std::string & version() const;

	void setName( const std::string & value );
	void setDescription( const std::string & value );
	void setVersion( const std::string & value );

	/*! Access the config to configure it.
	*/
	Config & config() const;

	/*! Access the commander to configure it.
	*/
	Commander & commander() const;

	/*! Logger
	*/
	std::shared_ptr<spdlog::logger> logger() const;

	/*! After configuring the app, call this method to parse options for all
		the selected sources.
	*/
	void parse();

	/*! The argumenst as passed in the App constructor
	*/
	std::vector<std::string> args() const;

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}}
