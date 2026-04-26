#pragma once

#include <functional>
#include <string>
#include <memory>
#include <string_view>
#include <vector>

#include "Export.h"

namespace spdlog {
	class logger;
}

namespace drea::core {

class App;
class Config;
class Commander;

/*! Callback type used to render a dynamic help footer. The first argument is
	the application; the second is the dotted name of the command whose help is
	being rendered, or an empty view for the top-level `--help`. Returning an
	empty string suppresses the footer for that call.
*/
using HelpFooterFn = std::function<std::string( const App &, std::string_view command )>;

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
	[[nodiscard]] static App & instance();

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

	/*! Install a callback that renders extra text appended to `--help` output.
		The callback is invoked for both the top-level help (`command` empty)
		and per-command help (dotted name). Return an empty string to skip the
		footer for that call. Pass an empty function to clear.
	*/
	void setHelpFooter( HelpFooterFn fn );

	/*! Compute the footer text for the given command (empty = top level).
		Returns an empty string when no callback is installed or it returned
		empty.
	*/
	[[nodiscard]] std::string helpFooter( std::string_view command ) const;

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}
