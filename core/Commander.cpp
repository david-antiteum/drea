#include <vector>
#include <algorithm>
#include <optional>
#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <woorm/levenshtein.h>

#include "Commander.h"
#include "App.h"
#include "Config.h"

#include "integrations/bash/bash_completion.h"
#include "integrations/help/help.h"
#include "utilities/string.h"

struct drea::core::Commander::Private
{
	std::string								mCommand;
	std::vector<std::string>				mArguments;
	std::vector<std::unique_ptr<Command>>	mCommands;

	jss::object_ptr<Command> find( const std::string & parent, const std::string & cmdName ){
		for( const auto & cmd: mCommands ){
			if( parent == cmd->mParentCommand && cmd->mName == cmdName ){
				return cmd;
			}
		}
		return {};
	}

	jss::object_ptr<Command> find( const std::string & cmdName ){
		auto 						commands = utilities::string::split( cmdName, "." );
		std::string 				parent;
		jss::object_ptr<Command>	res;

		for( const std::string & cmdName: commands ){
			auto cmd = find( parent, cmdName );
			if( cmd ){
				parent = cmd->mName;
				res = cmd;
			}else{
				return {};
			}
		}
		return res;
	}
};

drea::core::Commander::Commander()
{
	d = std::make_unique<Private>();
}

drea::core::Commander::~Commander()
{
}

void drea::core::Commander::commands( std::function<void(const drea::core::Command&)> f ) const
{	
	for( const auto & cmd: d->mCommands ){
		f( *cmd );
	}
}

void drea::core::Commander::addDefaults()
{
}

jss::object_ptr<drea::core::Command> drea::core::Commander::add( const drea::core::Command & cmd )
{
	d->mCommands.push_back( std::make_unique<Command>( cmd ));

	return d->mCommands.back();
}

void drea::core::Commander::configure( const std::vector<std::string> & args )
{
	if( !args.empty() ){
		if( auto cmd = find( args.at( 0 ) ) ){
			int	pos = 1;

			d->mCommand = cmd->mName;
			while( args.size() > pos ){
				auto it = std::find( cmd->mSubcommand.begin(), cmd->mSubcommand.end(), args.at( pos ) );
				if( it != cmd->mSubcommand.end() ){
					if( cmd = find( d->mCommand + "." + args.at( pos ))){
						d->mCommand += "." + cmd->mName;
						pos++;
					}else{
						break;
					}
				}else{
					break;
				}
			}
			for( int i = pos; i < args.size(); i++ ){
				d->mArguments.push_back( args[i] );
			}
		}else{
			reportNoCommand( args.at( 0 ) );
		}
	}
}

void drea::core::Commander::run( std::function<void( std::string )> f )
{
	if( App::instance().config().used( "version" )){
		drea::core::integrations::Help::version( App::instance() );
	}else if( App::instance().config().used( "generate-auto-completion" ) ){
		drea::core::integrations::Bash::generateAutoCompletion( App::instance() );
	}else if( App::instance().config().used( "help" ) ){
		if( d->mCommand.empty() ){
			drea::core::integrations::Help::help( App::instance() );
		}else{
			drea::core::integrations::Help::help( App::instance(), d->mCommand );
		}
	}else{
		f( d->mCommand );
	}
}

std::vector<std::string> drea::core::Commander::arguments() const
{
	return d->mArguments;
}

void drea::core::Commander::reportNoSubCommand( const std::string & command ) const
{
	if( auto cmd = find( command ) ){
		App::instance().logger().error( "The command \"{}\" requires a sub command. Try: {} {} --help", utilities::string::replace( command, ".", " " ), App::instance().args().at( 0 ), utilities::string::replace( command, ".", " " ) );
	}else{
		reportNoCommand( command );
	}
}

void drea::core::Commander::reportNoCommand( const std::string & command ) const
{
	if( command.empty() ){
		App::instance().logger().info( "A command is required." );
	}else{
		size_t			bestDist = 0;
		std::string		bestCmd;

		for( const auto & cmd: d->mCommands ){
			size_t	nd = levenshtein( command, cmd->mName );
			if( bestCmd.empty() || nd < bestDist ){
				bestDist = nd;
				bestCmd = cmd->mName;
			}
		}
		if( bestCmd.empty() ){
			App::instance().logger().error( "Unknown command \"{}\"", command );
		}else{
			App::instance().logger().error( "Unknown command \"{}\". Did you mean \"{}\"?", command, bestCmd );
		}
	}
}

jss::object_ptr<drea::core::Command> drea::core::Commander::find( const std::string & cmdName ) const
{
	return d->find( cmdName );
}

bool drea::core::Commander::empty() const
{
	return d->mCommands.empty();
}
