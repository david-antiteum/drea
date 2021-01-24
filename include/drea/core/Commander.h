#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <object_ptr/object_ptr.hpp>

#include "Export.h"
#include "Command.h"

namespace drea::core {

class App;

/*! Commands of the application
*/
class DREA_CORE_API Commander
{
public:
	explicit Commander( App & app );
	~Commander();

	/*! Adds defaults commands to the app
	*/
	Commander & addDefaults();

	/*! Adds a command to the app
	*/
	jss::object_ptr<Command> add( const drea::core::Command & cmd );

	/*! Adds commands to the app
	*/
	std::vector<jss::object_ptr<Command>> add( const std::vector<drea::core::Command> & cmds );

	/*! Use this method to be called with the command to execute.
		Do it after configuring the app and parsing the options (\see App::parse)
	*/
	void run( std::function<void( std::string )> f );

	/*! Get the arguments of the command
	*/
	[[nodiscard]] std::vector<std::string> arguments() const;

	/*! Any command?
	*/
	[[nodiscard]] bool empty() const;

	/*! Access the commands
	*/
	void commands( const std::function<void(const Command&)> & f ) const;

	/*! Find a command by name. Return nullptr if not found
	*/
	[[nodiscard]] jss::object_ptr<Command> find( std::string_view cmdName ) const;

	/*! Report to the user that this is not a valid command because is either unknown or because requires a missing subcommand.
		This method show a similar command if possible (Did you mean?).
	*/
	void unknownCommand( std::string_view command ) const;

	/*! Report to the user that the command needs a different number of arguments
	*/
	void wrongNumberOfArguments( std::string_view command ) const;

	// Methods called by App

	/*! Pass all the arguments that are not options to the commander to set it up.

		Don't call this method directly. App::parse will do it.
	*/
	void configure( const std::vector<std::string> & args );

	/*! Run the app in autocomplete mode

		Don't call this method directly. App::parse will do it.
	*/
	void configureForAutocompletion( const std::vector<std::string> & args );

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}
