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

	/*! Declare the positional arguments the app accepts when no command is
		given (the "root command"). Only the positional fields of \a cmd are
		used: mParamName, mNbParams, mMinParams, mParamChoices and mExamples.
		mName must be empty.

		With root params declared, a leading argument that is not a command is
		an argument of the app itself (\see arguments) and run() calls the
		callback with an empty command name. Without them, such an argument is
		a usage error.
	*/
	void setRoot( const drea::core::Command & cmd );

	/*! The root command, or nullptr when the app declares no root params.
	*/
	[[nodiscard]] jss::object_ptr<Command> root() const;

	/*! Remove a command (and its descendants) from the registry by dotted
		name. Useful to drop a builtin (`man`, `completion`, `describe`) that the app does
		not want to expose, or to retract a programmatically-added command.
		Removing a parent removes all its subcommands.
	*/
	void remove( std::string_view cmdName );

	/*! Use this method to be called with the command to execute.
		Do it after configuring the app and parsing the options (\see App::parse)

		A misuse of the command line — an unknown command, a command gated by
		disabled groups, the wrong number of arguments, an argument outside
		`param-choices` — is reported and quits with ExitCode::UsageError
		instead of calling \a f.
	*/
	void run( std::function<void( std::string )> f );

	/*! Replace how run() quits, which is std::exit by default.

		Meant for tests and for embedders that must not end the process. run()
		returns immediately after the handler, without dispatching, so a handler
		that returns normally is safe.
	*/
	void setExitHandler( std::function<void( int )> handler );

	/*! Get the arguments of the command
	*/
	[[nodiscard]] std::vector<std::string> arguments() const;

	/*! Any command?
	*/
	[[nodiscard]] bool empty() const;

	/*! Any command of the app itself, that is, ignoring the builtins added by
		addDefaults (man, completion)? Drives the usage line: an app with only
		builtins does not require a COMMAND.
	*/
	[[nodiscard]] bool hasAppCommands() const;

	/*! True when the arguments could not be dispatched: the first argument is
		not a command and the app declares no root params. run() refuses to
		call the callback and quits with ExitCode::UsageError.
	*/
	[[nodiscard]] bool invalidCommand() const;

	/*! The argument that could not be dispatched, empty when there was none.
		Config::findings reports it as an "unknown_command" finding, so
		--validate does not call a mistyped command line valid.
	*/
	[[nodiscard]] const std::string & invalidCommandName() const;

	/*! Access the commands
	*/
	void commands( const std::function<void(const Command&)> & f ) const;

	/*! Find a command by name. Return nullptr if not found
	*/
	[[nodiscard]] jss::object_ptr<Command> find( std::string_view cmdName ) const;

	/*! Set the list of groups currently enabled. A command is visible if it
		declares no group, or any of its groups is enabled. Empty list (default)
		means only commands without groups are visible.
	*/
	void setEnabledGroups( std::vector<std::string> groups );

	/*! Currently enabled groups (as set via setEnabledGroups).
	*/
	[[nodiscard]] const std::vector<std::string> & enabledGroups() const;

	/*! True if the command is visible: not flagged hidden, and either has no
		groups declared or at least one of its groups is enabled.
	*/
	[[nodiscard]] bool isVisible( const Command & cmd ) const;

	/*! Dotted name of the command parsed from argv (e.g. "cost.top"). Empty
		when no command was given.
	*/
	[[nodiscard]] const std::string & requestedCommand() const;

	/*! Report to the user that this is not a valid command because is either unknown or because requires a missing subcommand.
		This method show a similar command if possible (Did you mean?).
	*/
	void unknownCommand( std::string_view command ) const;

	/*! Report to the user that the command needs a different number of arguments
	*/
	void wrongNumberOfArguments( std::string_view command ) const;

	// Methods called by App

	/*! Pass all the arguments that are not options to the commander to set it up.

		A leading "--" forces the remaining arguments to be root params, even
		when the first of them is the name of a command.

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
