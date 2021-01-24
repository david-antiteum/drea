#pragma once

#include <string>
#include <memory>
#include <vector>

#include "Export.h"

namespace spdlog {
	class logger;
}

namespace drea::core {

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

	[[nodiscard]] const std::string & name() const;
	[[nodiscard]] const std::string & description() const;
	[[nodiscard]] const std::string & version() const;

	void setName( std::string_view value );
	void setDescription( std::string_view value );
	void setVersion( std::string_view value );

	/*! Access the config to configure it.
	*/
	[[nodiscard]] Config & config() const;

	/*! Access the commander to configure it.
	*/
	[[nodiscard]] Commander & commander() const;

	/*! Logger
	*/
	[[nodiscard]] spdlog::logger & logger() const;

	/*! After configuring the app, call this method to parse options for all
		the selected sources.
	*/
	void parse( const std::string & definitions );

	/*! Add definitions to parse
	*/
	void addToParser( std::string_view definitions );

	/*! After configuring the app, call this method to parse options for all
		the selected sources. Definitions have been added using addToParser method
	*/
	void parse();

	/*! The argumenst as passed in the App constructor
	*/
	[[nodiscard]] std::vector<std::string> args() const;

	/*! Set config values in runtime
	*/
	virtual void configureInRunTime(){};

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}
