#include "Commander.h"
#include "App.h"
#include "Config.h"

#include <vector>
#include <algorithm>
#include <optional>

#include <spdlog/fmt/fmt.h>

struct drea::core::Commander::Private
{
	std::string						mCommand;
	std::vector<std::string>		mArguments;
	std::vector<Command>			mCommands;

	std::optional<Command*> find( const std::string & cmdName ){
		std::optional<Command*>	res;

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

drea::core::Commander & drea::core::Commander::addDefaults()
{
	return *this;
}

drea::core::Commander & drea::core::Commander::add( drea::core::Command cmd )
{
	d->mCommands.push_back( cmd );

	return *this;
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
	if( App::instance().config().contains( "version" )){
		App::instance().showVersion();
	}else if( App::instance().config().contains( "help" ) ){
		if( d->mCommand.empty() ){
			App::instance().showHelp();
		}else{
			showHelp( d->mCommand );
		}
	}else if( !d->mCommand.empty() ){
		f( d->mCommand );
	}
}

std::vector<std::string> drea::core::Commander::arguments()
{
	return d->mArguments;
}

void drea::core::Commander::showHelp( const std::string & command )
{
	if( d->mCommands.empty() ){
		fmt::print( "This app has no commands\n" );
	}else{
		if( command.empty() ){
			fmt::print( "Available Commands:\n" );

			std::string::size_type offset = 1;
			for( const Command & command: d->mCommands ){
				offset = std::max<std::string::size_type>( offset, command.mName.size() + 2 );
			}
			for( const Command & command: d->mCommands ){
				fmt::print( "  {:<{}}", command.mName, offset );
				fmt::print( "{}\n", command.mDescription );
			}
		}else{
			auto cmdMaybe = d->find( command );

			if( cmdMaybe ){
				auto cmd = cmdMaybe.value();
				fmt::print( "{}\n\n", cmd->mDescription );

				fmt::print( "Usage:\n");
				fmt::print( "  {} {}\n", App::instance().name(), cmd->mName );

				if( !cmd->mLocalParameters.empty() ){
					fmt::print( "\nFlags:\n");
					for( const std::string & arg: cmd->mLocalParameters ){
						auto configMaybe = App::instance().config().find( arg );

						if( configMaybe ){
							fmt::print( "  --{} {}\n", configMaybe.value()->mName, configMaybe.value()->mDescription );
						}
					}
				}
				if( !cmd->mGlobalParameters.empty() ){
					fmt::print( "\nGlobal Flags:\n");
					for( const std::string & arg: cmd->mGlobalParameters ){
						auto configMaybe = App::instance().config().find( arg );

						if( configMaybe ){
							fmt::print( "  --{} {}\n", configMaybe.value()->mName, configMaybe.value()->mDescription );
						}
					}
				}				
			}
		}
	}
}
