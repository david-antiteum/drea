#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <object_ptr/object_ptr.hpp>

#include "Export.h"
#include "Command.h"

namespace drea { namespace core {
/*! Commands of the application
*/
class DREA_CORE_API Commander
{
public:
	explicit Commander();
	~Commander();

	/*! Adds defaults commands to the app
	*/
	void addDefaults();

	/*! Adds a command to the app
	*/
	void add( const drea::core::Command & cmd );

	/*! Use this method to be called with the command to execute.
		Do it after configuring the app and parsing the options (\see App::parse)
	*/
	void run( std::function<void( std::string )> f );

	/*! Get the arguments of the command
	*/
	std::vector<std::string> arguments() const;

	/*! Any command?
	*/
	bool empty() const;

	/*! Access the commands
	*/
	void commands( std::function<void(const Command&)> f ) const;

	/*! Find a command by name. Return nullptr if not found
	*/
	jss::object_ptr<Command> find( const std::string & cmdName ) const;

	/*! Report to the user that this is not a valid command. This method show a similar command if possible (Did you mean?).
	*/
	void reportNoCommand( const std::string & command ) const;

	// Methods called by App

	/*! Pass all the arguments that are not options to the commander to set it up.

		Don't call this method directly. App::parse will do it.
	*/
	void configure( const std::vector<std::string> & args );

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}}