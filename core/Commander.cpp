#include <vector>
#include <algorithm>
#include <optional>
#include <memory>
#include <set>
#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <woorm/levenshtein.h>

#include "Commander.h"
#include "App.h"
#include "Config.h"

#include "integrations/help/help.h"
#include "integrations/bash/bash_completion.h"
#include "integrations/zsh/zsh_completion.h"
#include "integrations/fish/fish_completion.h"
#include "integrations/man/man.h"
#include "utilities/string.h"

struct drea::core::Commander::Private
{
	std::string								mCommand;
	std::vector<std::string>				mArguments;
	std::vector<std::unique_ptr<Command>>	mCommands;
	std::set<std::string>					mBuiltins;
	std::vector<std::string>				mEnabledGroups;
	App										& mApp;

	explicit Private( App & app ) : mApp( app )
	{
	}

	bool isVisible( const Command & cmd ) const
	{
		if( cmd.mHidden ){
			return false;
		}
		if( cmd.mGroups.empty() ){
			return true;
		}
		for( const auto & g: cmd.mGroups ){
			if( std::find( mEnabledGroups.begin(), mEnabledGroups.end(), g ) != mEnabledGroups.end() ){
				return true;
			}
		}
		return false;
	}

	jss::object_ptr<Command> find( const std::string & parent, const std::string & cmdName ){
		for( const auto & cmd: mCommands ){
			if( parent == cmd->mParentCommand && cmd->mName == cmdName ){
				return cmd;
			}
		}
		return {};
	}

	jss::object_ptr<Command> find( std::string_view fullCmdName ){
		auto 						commands = utilities::string::split( fullCmdName, "." );
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
		// Group inheritance: a child with no declared groups inherits its
		// parent's groups. Walk to a fixed point so multi-level chains
		// resolve regardless of insertion order.
		bool changed = true;
		while( changed ){
			changed = false;
			for( const auto & cmd: mCommands ){
				if( cmd->mGroups.empty() && !cmd->mParentCommand.empty() ){
					if( auto parent = find( cmd->mParentCommand ); parent && !parent->mGroups.empty() ){
						cmd->mGroups = parent->mGroups;
						changed = true;
					}
				}
			}
		}
	}

	void detectLocalOptionCollisions()
	{
		std::vector<std::pair<std::string, std::vector<std::string>>> optionToCommands;
		for( const auto & cmd: mCommands ){
			for( const auto & opt: cmd->mLocalParameters ){
				auto it = std::find_if( optionToCommands.begin(), optionToCommands.end(),
					[&opt]( const auto & p ){ return p.first == opt; } );
				if( it == optionToCommands.end() ){
					optionToCommands.push_back( { opt, { cmd->mName } } );
				}else{
					it->second.push_back( cmd->mName );
				}
			}
		}
		for( const auto & entry: optionToCommands ){
			if( entry.second.size() > 1 ){
				mApp.logger().debug( "Option \"{}\" used as local-option in multiple commands: {}", entry.first, utilities::string::join( entry.second, ", " ) );
			}
		}
	}
};

drea::core::Commander::Commander( drea::core::App & app ) : d( std::make_unique<Private>( app ) )
{
}

drea::core::Commander::~Commander() = default;

void drea::core::Commander::commands( const std::function<void(const drea::core::Command&)> & f ) const
{	
	for( const auto & cmd: d->mCommands ){
		f( *cmd );
	}
}

drea::core::Commander & drea::core::Commander::addDefaults()
{
	auto hasCommand = [this]( std::string_view name ){
		for( const auto & c: d->mCommands ){
			if( c->mParentCommand.empty() && c->mName == name ){
				return true;
			}
		}
		return false;
	};

	if( !hasCommand( "completion" ) ){
		Command cmd;
		cmd.mName = "completion";
		cmd.mDescription = "Print a shell completion script";
		cmd.mParamName = "shell";
		cmd.mNbParams = 1;
		cmd.mMinParams = 0;
		add( cmd );
		d->mBuiltins.insert( "completion" );
	}
	if( !hasCommand( "man" ) ){
		Command cmd;
		cmd.mName = "man";
		cmd.mDescription = "Print a man page for this application";
		cmd.mParamName = "command";
		cmd.mNbParams = 1;
		cmd.mMinParams = 0;
		add( cmd );
		d->mBuiltins.insert( "man" );
	}
	return *this;
}

static bool runBuiltin( drea::core::App & app, const std::string & cmd, const std::vector<std::string> & args )
{
	if( cmd == "completion" ){
		std::string shell = args.empty() ? std::string( "bash" ) : args.front();
		if( shell == "bash" ){
			drea::core::integrations::Bash::generateAutoCompletion( app, std::cout );
		}else if( shell == "zsh" ){
			drea::core::integrations::Zsh::generateAutoCompletion( app, std::cout );
		}else if( shell == "fish" ){
			drea::core::integrations::Fish::generateAutoCompletion( app, std::cout );
		}else{
			app.logger().error( "Unsupported shell \"{}\". Supported: bash, zsh, fish.", shell );
			return true;
		}
		return true;
	}
	if( cmd == "man" ){
		if( args.empty() ){
			drea::core::integrations::Man::generateManPage( app, std::cout );
		}else{
			std::string target = args.front();
			std::replace( target.begin(), target.end(), ' ', '.' );
			drea::core::integrations::Man::generateManPage( app, target, std::cout );
		}
		return true;
	}
	return false;
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
		for( size_t i = 2; i < args.size(); i++ ){
			d->mArguments.push_back( args[i] );
		}
	}
}

void drea::core::Commander::configure( const std::vector<std::string> & args )
{
	d->createHierarchy();
	d->detectLocalOptionCollisions();
	if( !args.empty() ){
		if( auto cmd = find( args.at( 0 ) ); cmd ){
			int	pos = 1;

			d->mCommand = cmd->mName;
			while( int(args.size()) > pos ){
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
			for( size_t i = pos; i < args.size(); i++ ){
				d->mArguments.push_back( args[i] );
			}
		}else{
			unknownCommand( args.at( 0 ) );
		}
	}
}

void drea::core::Commander::run( std::function<void( std::string )> f )
{
	// Visibility gate: a command whose groups are not enabled (or that has
	// been hidden) must be indistinguishable from a typo. This covers both
	// `myapp gated --help` and direct invocation `myapp gated arg`.
	if( !d->mCommand.empty() ){
		if( auto cmd = find( d->mCommand ); cmd && !d->isVisible( *cmd ) ){
			unknownCommand( d->mCommand );
			return;
		}
	}
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
		if( !d->mCommand.empty() ){
			if( auto cmd = find( d->mCommand ) ){
				const int maxP = cmd->maxParams();
				const int minP = cmd->minParams();
				const int actual = static_cast<int>( d->mArguments.size() );
				if( maxP != drea::core::Command::mUnlimitedParams ){
					if( actual < minP || actual > maxP ){
						wrongNumberOfArguments( d->mCommand );
						return;
					}
				}else if( actual < minP ){
					wrongNumberOfArguments( d->mCommand );
					return;
				}
			}
		}
		if( !d->mCommand.empty() && d->mBuiltins.count( d->mCommand ) ){
			if( runBuiltin( d->mApp, d->mCommand, d->mArguments ) ){
				return;
			}
		}
		f( d->mCommand );
	}
}

std::vector<std::string> drea::core::Commander::arguments() const
{
	return d->mArguments;
}

void drea::core::Commander::unknownCommand( std::string_view command ) const
{
	if( command.empty() ){
		d->mApp.logger().info( "A command is required." );
	}else if( auto found = find( command ); found && d->isVisible( *found ) ){
		d->mApp.logger().error( "The command \"{}\" requires a sub command. Try: {} {} --help", utilities::string::replace( command, ".", " " ), d->mApp.args().at( 0 ), utilities::string::replace( command, ".", " " ) );
	}else{
		size_t			bestDist = 0;
		std::string		bestCmd;

		for( const auto & cmd: d->mCommands ){
			if( cmd->mParentCommand.empty() && d->isVisible( *cmd ) ){
				size_t	nd = levenshtein( command, cmd->mName );
				if( bestCmd.empty() || nd < bestDist ){
					bestDist = nd;
					bestCmd = cmd->mName;
				}
			}
		}
		if( bestCmd.empty() ){
			d->mApp.logger().error( "Unknown command \"{}\"", utilities::string::replace( command, ".", " " ) );
		}else{
			d->mApp.logger().error( "Unknown command \"{}\". Did you mean \"{}\"?", utilities::string::replace( command, ".", " " ), bestCmd );
		}
	}
}

void drea::core::Commander::wrongNumberOfArguments( std::string_view command ) const
{
	if( auto cmd = find( command ) ){
		if( cmd->mNbParams == drea::core::Command::mUnlimitedParams ){
			d->mApp.logger().error( "The command \"{}\" requires at least one argument.", utilities::string::replace( command, ".", " " ) );
		}else if( cmd->mNbParams == 0 ){
			d->mApp.logger().error( "The command \"{}\" has no arguments.", utilities::string::replace( command, ".", " " ) );
		}else{
			d->mApp.logger().error( "The command \"{}\" requires {} argument{}, {} given.", utilities::string::replace( command, ".", " " ), cmd->mNbParams, cmd->mNbParams > 1 ? "s": "", arguments().size() );
		}
	}
}

jss::object_ptr<drea::core::Command> drea::core::Commander::find( std::string_view cmdName ) const
{
	return d->find( cmdName );
}

void drea::core::Commander::setEnabledGroups( std::vector<std::string> groups )
{
	d->mEnabledGroups = std::move( groups );
}

const std::vector<std::string> & drea::core::Commander::enabledGroups() const
{
	return d->mEnabledGroups;
}

bool drea::core::Commander::isVisible( const drea::core::Command & cmd ) const
{
	return d->isVisible( cmd );
}

const std::string & drea::core::Commander::requestedCommand() const
{
	return d->mCommand;
}

bool drea::core::Commander::empty() const
{
	return d->mCommands.empty();
}
