#include <vector>
#include <algorithm>
#include <optional>
#include <functional>
#include <memory>
#include <set>
#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <woorm/levenshtein.h>

#include "Commander.h"
#include "App.h"
#include "Config.h"
#include "ExitCode.h"

#include "integrations/help/help.h"
#include "integrations/help/describe.h"
#include "integrations/help/validate.h"
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
	std::unique_ptr<Command>				mRoot;
	std::set<std::string>					mBuiltins;
	std::vector<std::string>				mEnabledGroups;
	bool									mInvalidCommand = false;
	std::string								mInvalidCommandName;
	std::function<void( int )>				mExit = []( int code ){ std::exit( code ); };
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

	const std::string & appName = d->mApp.name();

	if( !hasCommand( "completion" ) ){
		Command cmd;
		cmd.mName = "completion";
		cmd.mDescription = fmt::format(
			"Print a shell completion script for {0} to stdout.\n"
			"Source it in the current shell, or install it where the shell looks\n"
			"for completions. Defaults to bash when no shell is given.", appName );
		cmd.mParamName = "shell";
		cmd.mNbParams = 1;
		cmd.mMinParams = 0;
		cmd.mParamChoices = { "bash", "zsh", "fish" };
		cmd.mExamples = {
			fmt::format( "eval \"$({0} completion bash)\"", appName ),
			fmt::format( "{0} completion bash > /etc/bash_completion.d/{0}", appName ),
			fmt::format( "{0} completion zsh > \"${{fpath[1]}}/_{0}\"", appName ),
			fmt::format( "{0} completion fish > ~/.config/fish/completions/{0}.fish", appName )
		};
		cmd.mPredefined = true;
		add( cmd );
		d->mBuiltins.insert( "completion" );
	}
	if( !hasCommand( "describe" ) ){
		Command cmd;
		cmd.mName = "describe";
		cmd.mDescription = fmt::format(
			"Print the description of {0} as JSON to stdout.\n"
			"Commands, subcommands, options, types, bounds, choices and limits:\n"
			"everything a tool needs to drive {0} without reading --help.", appName );
		cmd.mNbParams = 0;
		cmd.mExamples = {
			fmt::format( "{0} describe | jq .commands", appName ),
			fmt::format( "{0} describe > {0}-cli.json", appName )
		};
		cmd.mPredefined = true;
		add( cmd );
		d->mBuiltins.insert( "describe" );
	}
	if( !hasCommand( "man" ) ){
		Command cmd;
		cmd.mName = "man";
		cmd.mDescription = fmt::format(
			"Print a man page (roff) for {0} to stdout.\n"
			"With a command, print the page of that command only. Quote a\n"
			"subcommand path, as in \"config set\".", appName );
		cmd.mParamName = "command";
		cmd.mNbParams = 1;
		cmd.mMinParams = 0;
		cmd.mExamples = {
			fmt::format( "{0} man | man -l -", appName ),
			fmt::format( "{0} man > {0}.1", appName )
		};
		cmd.mPredefined = true;
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
	if( cmd == "describe" ){
		drea::core::integrations::Help::describe( app, std::cout );
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

void drea::core::Commander::setRoot( const drea::core::Command & cmd )
{
	d->mRoot = std::make_unique<Command>( cmd );
	d->mRoot->mName.clear();
	d->mRoot->mParentCommand.clear();
	d->mRoot->mSubcommand.clear();
}

jss::object_ptr<drea::core::Command> drea::core::Commander::root() const
{
	return d->mRoot ? jss::object_ptr<Command>( d->mRoot.get() ) : jss::object_ptr<Command>();
}

void drea::core::Commander::remove( std::string_view cmdName )
{
	const std::string	target( cmdName );
	auto isDescendant = [&target]( const std::string & path ){
		return path == target || ( path.size() > target.size()
			&& path.compare( 0, target.size(), target ) == 0
			&& path[target.size()] == '.' );
	};
	d->mCommands.erase(
		std::remove_if( d->mCommands.begin(), d->mCommands.end(),
			[&]( const std::unique_ptr<Command> & cmd ){
				const std::string fullPath = cmd->mParentCommand.empty()
					? cmd->mName
					: cmd->mParentCommand + "." + cmd->mName;
				return isDescendant( fullPath );
			} ),
		d->mCommands.end() );
	d->mBuiltins.erase( target );
	// The cached subcommand lists on parents may now reference removed
	// children; rebuild them.
	for( const auto & cmd: d->mCommands ){
		cmd->mSubcommand.clear();
	}
	d->createHierarchy();
}

void drea::core::Commander::configure( const std::vector<std::string> & rawArgs )
{
	d->createHierarchy();
	d->detectLocalOptionCollisions();
	d->mInvalidCommand = false;
	d->mInvalidCommandName.clear();
	if( !rawArgs.empty() && rawArgs.at( 0 ) == "--" ){
		// explicit end of commands: everything left is a root param, even if
		// it happens to be the name of a command
		for( size_t i = 1; i < rawArgs.size(); i++ ){
			d->mArguments.push_back( rawArgs[i] );
		}
		if( !d->mRoot ){
			d->mInvalidCommand = true;
			d->mInvalidCommandName = "--";
			d->mApp.logger().error( "The application \"{}\" takes no arguments of its own, only commands.", d->mApp.name() );
		}
		return;
	}
	// a "--" after the command only marks the end of the options: it is not an
	// argument of the command
	std::vector<std::string>	args;
	args.reserve( rawArgs.size() );
	for( const auto & arg: rawArgs ){
		if( arg != "--" ){
			args.push_back( arg );
		}
	}
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
		}else if( d->mRoot ){
			// no command given: the app itself takes these arguments
			d->mArguments = args;
		}else{
			d->mInvalidCommand = true;
			d->mInvalidCommandName = args.at( 0 );
			unknownCommand( args.at( 0 ) );
		}
	}
}

void drea::core::Commander::setExitHandler( std::function<void( int )> handler )
{
	if( handler ){
		d->mExit = std::move( handler );
	}
}

void drea::core::Commander::run( std::function<void( std::string )> f )
{
	// Every way out of run() other than dispatching goes through here, so a
	// misuse of the command line always reaches the shell as an exit code and
	// tests can observe the code without ending the process (\see
	// setExitHandler). The caller returns right after.
	auto quit = [ this ]( ExitCode code ){
		d->mApp.logger().flush();
		d->mExit( toInt( code ) );
	};

	// --validate is a mode of its own: check the resolved configuration,
	// report and quit with the mapped exit code. It runs before the
	// visibility gate so a command gated by disabled groups is reported as
	// a finding, and before --help and --version so the exit code always
	// reflects the check.
	if( d->mApp.config().used( "validate" ) ){
		// the value, not the presence: --no-json asks for human output
		const int code = integrations::Help::validateConfig( d->mApp, d->mApp.config().get<bool>( "json" ) );

		d->mExit( code );
		return;
	}
	// Visibility gate: a command whose groups are not enabled (or that has
	// been hidden) must be indistinguishable from a typo. This covers both
	// `myapp gated --help` and direct invocation `myapp gated arg`.
	if( !d->mCommand.empty() ){
		if( auto cmd = find( d->mCommand ); cmd && !d->isVisible( *cmd ) ){
			unknownCommand( d->mCommand );
			quit( ExitCode::UsageError );
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
	}else{
		// An argument that is not a command, in an app that declares no root
		// params, is a usage error: never dispatch it to the app. Checked
		// after --help and --version so those keep working on a typo.
		if( d->mInvalidCommand ){
			quit( ExitCode::UsageError );
			return;
		}
		// the root command carries the params accepted with no command given
		if( auto cmd = d->mCommand.empty() ? root() : find( d->mCommand ); cmd ){
			const int maxP = cmd->maxParams();
			const int minP = cmd->minParams();
			const int actual = static_cast<int>( d->mArguments.size() );
			if( maxP != drea::core::Command::mUnlimitedParams ){
				if( actual < minP || actual > maxP ){
					wrongNumberOfArguments( d->mCommand );
					quit( ExitCode::UsageError );
					return;
				}
			}else if( actual < minP ){
				wrongNumberOfArguments( d->mCommand );
				quit( ExitCode::UsageError );
				return;
			}
			// param-choices: the positional argument must be one of the
			// declared values, same rule as choices on options
			if( !cmd->mParamChoices.empty() ){
				for( const auto & argument: d->mArguments ){
					if( std::find( cmd->mParamChoices.begin(), cmd->mParamChoices.end(), argument ) == cmd->mParamChoices.end() ){
						d->mApp.logger().error( "The {} argument \"{}\" is not one of: {}",
							d->mCommand.empty()
								? fmt::format( "application \"{}\"", d->mApp.name() )
								: fmt::format( "command \"{}\"", utilities::string::replace( d->mCommand, ".", " " ) ),
							argument, utilities::string::join( cmd->mParamChoices, ", " ) );
						quit( ExitCode::UsageError );
						return;
					}
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
	// an empty name means the root command: the params the app takes itself
	if( auto cmd = command.empty() ? root() : find( command ) ){
		const std::string subject = command.empty()
			? fmt::format( "application \"{}\"", d->mApp.name() )
			: fmt::format( "command \"{}\"", utilities::string::replace( command, ".", " " ) );

		if( cmd->mNbParams == drea::core::Command::mUnlimitedParams ){
			d->mApp.logger().error( "The {} requires at least {} argument{}.", subject, std::max( 1, cmd->minParams() ), cmd->minParams() > 1 ? "s": "" );
		}else if( cmd->mNbParams == 0 ){
			d->mApp.logger().error( "The {} has no arguments.", subject );
		}else{
			d->mApp.logger().error( "The {} requires {} argument{}, {} given.", subject, cmd->mNbParams, cmd->mNbParams > 1 ? "s": "", arguments().size() );
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

bool drea::core::Commander::hasAppCommands() const
{
	for( const auto & cmd: d->mCommands ){
		if( cmd->mParentCommand.empty() && !cmd->mPredefined && d->isVisible( *cmd ) ){
			return true;
		}
	}
	return false;
}

bool drea::core::Commander::invalidCommand() const
{
	return d->mInvalidCommand;
}

const std::string & drea::core::Commander::invalidCommandName() const
{
	return d->mInvalidCommandName;
}
