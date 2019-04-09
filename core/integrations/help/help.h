#pragma once

#include <spdlog/fmt/fmt.h>
#include <iostream>
#include <fstream>

#include "App.h"
#include "Commander.h"
#include "Config.h"
#include "utilities/string.h"

namespace drea { namespace core { namespace integrations { namespace Help {

static void version( const drea::core::App & app )
{
	fmt::print( "{} version {}\n", app.name(), app.version() );
}

static void help( const drea::core::App & app, const std::string & command )
{
	if( app.commander().empty() ){
		fmt::print( "This app has no commands\n" );
	}else{
		if( command.empty() ){
			fmt::print( "Commands:\n" );

			std::string::size_type offset = 1;
			app.commander().commands( [ &offset ](const Command & command ){
				offset = std::max<std::string::size_type>( offset, command.mName.size() + 2 );
			});

			app.commander().commands( [ offset ](const Command & command ){
				if( command.mParentCommand.empty() ){
					fmt::print( "  {:<{}}", command.mName, offset );
					if( !command.mSubcommand.empty() ){
						fmt::print( "[command] " );
					}
					fmt::print( "{}\n", command.mDescription );
				}
			});

			fmt::print( "\nUse \"{} [command] --help\" for more information about a command.\n", app.name() );
		}else{
			if( auto cmd = app.commander().find( command ) ){
				auto commands = utilities::string::split( command, "." );

				fmt::print( "\nUsage:");
				fmt::print( "  {} {} ", App::instance().name(), utilities::string::join( commands, " " ));
				if( !cmd->mSubcommand.empty() ){
					fmt::print( "[command] " );
				}
				fmt::print( "[<args>] " );
				if( !cmd->mLocalParameters.empty() || !cmd->mGlobalParameters.empty() ){
					fmt::print( "[<flags>]" );
				}
				fmt::print( "\n\n" );

				fmt::print( "{}\n", cmd->mDescription );

				if( !cmd->mSubcommand.empty() ){
					fmt::print( "\nCommands:\n");
					for( const std::string & subCmdName: cmd->mSubcommand ){
						if( auto subCmd = App::instance().commander().find( command + "." + subCmdName ) ){
							fmt::print( "  {} {}\n", subCmd->mName, subCmd->mDescription );
						}
					}
				}

				if( !cmd->mLocalParameters.empty() ){
					fmt::print( "\nFlags:\n");
					for( const std::string & arg: cmd->mLocalParameters ){
						if( auto config = App::instance().config().find( arg ) ){
							fmt::print( "  --{} {}\n", config->mName, config->mDescription );
						}
					}
				}
				if( !cmd->mGlobalParameters.empty() ){
					fmt::print( "\nGlobal Flags:\n");
					for( const std::string & arg: cmd->mGlobalParameters ){
						if( auto config = App::instance().config().find( arg ) ){
							fmt::print( "  --{} {}\n", config->mName, config->mDescription );
						}
					}
				}
				if( !cmd->mSubcommand.empty() ){
					fmt::print( "\nUse \"{} {} [command] --help\" for more information about a command.\n", app.name(), utilities::string::join( commands, " " ));
				}						
			}
		}
	}
}

static void help( const drea::core::App & app )
{
	const std::string::size_type offset = std::string( "usage: " + app.name() + " " ).size();

	fmt::print( "\n{}\n\n", app.description() );
	fmt::print("usage: {} [command] [<args>]\n", app.name() );

	// Config
	app.config().options( [ offset ](const Option & option){
		for( int i = 0; i < offset; i++ ){
			std::cout << " ";
		}
		if( option.mParamName.empty() ){
			fmt::print( "[--{}]", option.mName );
		}else{
			fmt::print( "[--{} <{}>]", option.mName, option.mParamName );
		}
		fmt::print( " {}", option.mDescription );
		if( option.mValues.empty() ){
			fmt::print( "\n" );
		}else{
			fmt::print( ". Default" );
			for( auto v: option.mValues ){
				fmt::print( " {}", option.toString( v ));
			}
			fmt::print( "\n" );
		}
	});
	fmt::print( "\n" );

	// Commander
	help( app, {} );
}

}}}}
