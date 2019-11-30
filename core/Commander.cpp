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

#include "integrations/help/help.h"
#include "integrations/bash/bash_completion.h"
#include "utilities/string.h"

struct drea::core::Commander::Private
{
	std::string								mCommand;
	std::vector<std::string>				mArguments;
	std::vector<std::unique_ptr<Command>>	mCommands;
	App										& mApp;

	Private( App & app ) : mApp( app )
	{
	}

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
				if( parent.empty() ){
					parent = cmd->mName;
				}else{
					parent += "." + cmd->mName;
				}
				res = cmd;
			}else{
				return {};
			}
		}
		return res;
	}

	void createHierarchy()
	{
		for( const auto & cmd: mCommands ){
			if( !cmd->mParentCommand.empty() ){
				if( auto parent = find( cmd->mParentCommand ) ){
					parent->mSubcommand.push_back( cmd->mName );
				}else{
					mApp.logger().error( "The command \"{}\" refers to the parent \"{}\" but it does not exists.", cmd->mName, cmd->mParentCommand );
				}
			}
		}
	}
};

drea::core::Commander::Commander( drea::core::App & app ) : d( std::make_unique<Private>( app ) )
{
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

drea::core::Commander & drea::core::Commander::addDefaults()
{
	return *this;
}

jss::object_ptr<drea::core::Command> drea::core::Commander::add( const drea::core::Command & cmd )
{
	d->mCommands.push_back( std::make_unique<Command>( cmd ));

	return d->mCommands.back();
}

std::vector<jss::object_ptr<drea::core::Command>> drea::core::Commander::add( const std::vector<drea::core::Command> & cmds )
{
	std::vector<jss::object_ptr<drea::core::Command>>		res;

	for( const auto & cmd: cmds ){
		res.push_back( add( cmd ) );
	}
	return res;
}

void drea::core::Commander::configureForAutocompletion( const std::vector<std::string> & args )
{
	d->createHierarchy();
	if( args.size() > 1 ){
		d->mCommand = "autocomplete";
		for( int i = 2; i < args.size(); i++ ){
			d->mArguments.push_back( args[i] );
		}
	}
}

void drea::core::Commander::configure( const std::vector<std::string> & args )
{
	d->createHierarchy();
	if( !args.empty() ){
		if( auto cmd = find( args.at( 0 ) ); cmd ){
			int	pos = 1;

			d->mCommand = cmd->mName;
			while( args.size() > pos ){
				if( auto it = std::find( cmd->mSubcommand.begin(), cmd->mSubcommand.end(), args.at( pos ) ); it != cmd->mSubcommand.end() ){
					cmd = find( d->mCommand + "." + args.at( pos ));
					if( cmd ){
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
			unknownCommand( args.at( 0 ) );
		}
	}
}

void drea::core::Commander::run( std::function<void( std::string )> f )
{
	if( d->mApp.config().used( "version" )){
		drea::core::integrations::Help::version( d->mApp );
	}else if( d->mApp.config().used( "help" ) ){
		if( d->mCommand.empty() ){
			drea::core::integrations::Help::help( d->mApp );
		}else{
			drea::core::integrations::Help::help( d->mApp, d->mCommand );
		}
	}else if( d->mCommand == "autocomplete" ){
		for( auto var: drea::core::integrations::Bash::calculateAutoCompletion( d->mApp )){
			fmt::print( "{}\n", var );
		}
	}else{
		f( d->mCommand );
	}
}

std::vector<std::string> drea::core::Commander::arguments() const
{
	return d->mArguments;
}

void drea::core::Commander::unknownCommand( const std::string & command ) const
{
	if( command.empty() ){
		d->mApp.logger().info( "A command is required." );
	}else if( auto cmd = find( command ) ){
		d->mApp.logger().error( "The command \"{}\" requires a sub command. Try: {} {} --help", utilities::string::replace( command, ".", " " ), d->mApp.args().at( 0 ), utilities::string::replace( command, ".", " " ) );
	}else{
		size_t			bestDist = 0;
		std::string		bestCmd;

		for( const auto & cmd: d->mCommands ){
			if( cmd->mParentCommand.empty() ){
				size_t	nd = levenshtein( command, cmd->mName );
				if( bestCmd.empty() || nd < bestDist ){
					bestDist = nd;
					bestCmd = cmd->mName;
				}
			}
		}
		if( bestCmd.empty() ){
			d->mApp.logger().error( "Unknown command \"{}\"", command );
		}else{
			d->mApp.logger().error( "Unknown command \"{}\". Did you mean \"{}\"?", command, bestCmd );
		}
	}
}

void drea::core::Commander::wrongNumberOfArguments( const std::string & command ) const
{
	if( auto cmd = find( command ) ){
		d->mApp.logger().error( "The command \"{}\" requires {} command{}, {} given.", utilities::string::replace( command, ".", " " ), cmd->mNbParams, cmd->mNbParams > 1 ? "s": "", arguments().size() );
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
