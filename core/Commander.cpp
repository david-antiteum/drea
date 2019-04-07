#include "Commander.h"
#include "App.h"
#include "Config.h"

#include <vector>
#include <algorithm>
#include <optional>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <woorm/levenshtein.h>

#include "integrations/bash/bash_completion.h"
#include "integrations/help/help.h"

struct drea::core::Commander::Private
{
	std::string						mCommand;
	std::vector<std::string>		mArguments;
	std::vector<Command>			mCommands;

	jss::object_ptr<Command> find( const std::string & cmdName ){
		jss::object_ptr<Command>	res;

		for( Command & cmd: mCommands ){
			if( cmd.mName == cmdName ){
				res = &cmd;
				break;
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
	for( const Command & cmd: d->mCommands ){
		f( cmd );
	}
}

void drea::core::Commander::addDefaults()
{
}

void drea::core::Commander::add( drea::core::Command cmd )
{
	d->mCommands.push_back( cmd );
}

void drea::core::Commander::configure( const std::vector<std::string> & args )
{
	if( !args.empty() ){
		d->mCommand = args[0];
		for( int i = 1; i < args.size(); i++ ){
			if( args[i][0] != '-' ){
				d->mArguments.push_back( args[i] );
			}
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

void drea::core::Commander::reportNoCommand( const std::string & command ) const
{
	if( command.empty() ){
		App::instance().logger()->info( "A command is required." );
	}else{
		size_t			bestDist = 0;
		std::string		bestCmd;

		for( const Command & cmd: d->mCommands ){
			size_t	nd = levenshtein( command, cmd.mName );
			if( bestCmd.empty() || nd < bestDist ){
				bestDist = nd;
				bestCmd = cmd.mName;
			}
		}
		if( bestCmd.empty() ){
			App::instance().logger()->error( "Unknown command \"{}\"", command );
		}else{
			App::instance().logger()->error( "Unknown command \"{}\". Did you mean \"{}\"?", command, bestCmd );
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
