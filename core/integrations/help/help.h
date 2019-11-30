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

			std::string::size_type offset = 0;
			bool anySubCmd = false;

			app.commander().commands( [ &offset, &anySubCmd ](const Command & command ){
				offset = std::max<std::string::size_type>( offset, command.mName.size() + 2 );
				if( !command.mSubcommand.empty() ){
					anySubCmd = true;
				}
			});
			if( anySubCmd ){
				offset += 8;
			}
			app.commander().commands( [ offset ](const Command & command ){
				if( command.mParentCommand.empty() ){
					std::string::size_type cmdSize = 2 + command.mName.size();
					fmt::print( "  {}", command.mName );
					if( !command.mSubcommand.empty() ){
						fmt::print( " COMMAND" );
						cmdSize += 8;
					}
					fmt::print("{:>{}}", "", 2 + offset - cmdSize );
					fmt::print( "{}\n", command.mDescription );
				}
			});

			fmt::print( "\nUse \"{} COMMAND --help\" for more information about a command.\n", app.name() );
		}else{
			if( auto cmd = app.commander().find( command ) ){
				auto commands = utilities::string::split( command, "." );

				fmt::print( "\nusage:");
				fmt::print( " {} {} ", app.name(), utilities::string::join( commands, " " ));
				if( !cmd->mSubcommand.empty() ){
					fmt::print( "COMMAND " );
				}
				if( cmd->numberOfParams() > 0 ){
					fmt::print( "[ARGS] " );
				}
				if( !cmd->mLocalParameters.empty() || !cmd->mGlobalParameters.empty() ){
					fmt::print( "[OPTIONS]" );
				}
				fmt::print( "\n\n" );

				fmt::print( "{}\n", cmd->mDescription );

				if( !cmd->mSubcommand.empty() ){
					std::string::size_type offset = 0;
					bool anySubCmd = false;
				
					for( const std::string & subCmdName: cmd->mSubcommand ){
						if( auto subCmd = app.commander().find( command + "." + subCmdName ) ){
							offset = std::max<std::string::size_type>( offset, subCmd->mName.size() + 2 );
							if( !subCmd->mSubcommand.empty() ){
								anySubCmd = true;
							}
						}
					}
					if( anySubCmd ){
						offset += 8;
					}
					fmt::print( "\nCommands:\n");
					for( const std::string & subCmdName: cmd->mSubcommand ){
						if( auto subCmd = app.commander().find( command + "." + subCmdName ) ){
							std::string::size_type cmdSize = 2 + subCmd->mName.size();
					
							fmt::print( "  {}", subCmd->mName );
							if( !subCmd->mSubcommand.empty() ){
								fmt::print( " COMMAND" );
								cmdSize += 8;
							}
							fmt::print("{:>{}}", "", 2 + offset - cmdSize );
							fmt::print( "{}\n", subCmd->mDescription );
						}
					}
				}

				if( !cmd->mLocalParameters.empty() ){
					fmt::print( "\nOptions:\n");
					for( const std::string & arg: cmd->mLocalParameters ){
						if( auto config = app.config().find( arg ) ){
							fmt::print( "  --{} {}\n", config->mName, config->mDescription );
						}
					}
				}
				if( !cmd->mGlobalParameters.empty() ){
					fmt::print( "\nGlobal options:\n");
					for( const std::string & arg: cmd->mGlobalParameters ){
						if( auto config = app.config().find( arg ) ){
							fmt::print( "  --{} {}\n", config->mName, config->mDescription );
						}
					}
				}
				if( !cmd->mSubcommand.empty() ){
					fmt::print( "\nUse \"{} {} COMMAND --help\" for more information about a command.\n", app.name(), utilities::string::join( commands, " " ));
				}						
			}
		}
	}
}

static void help( const drea::core::App & app )
{
	std::string::size_type 	offset = 0;
	bool					anyShort = false;

	fmt::print( "\n{}\n", app.description() );
	fmt::print("usage: {}", app.name() );
	if( !app.commander().empty() ){
		fmt::print(" COMMAND", app.name() );
	}
	fmt::print(" [OPTIONS]\n\n", app.name() );

	app.config().options( [ &offset, &anyShort ](const Option & option){
		std::string::size_type optionOffset = 2 + 2 + option.mName.size();
		if( !option.mShortVersion.empty() ){
			anyShort = true;
		}
		if( !option.mParamName.empty() ){
			optionOffset += 1 + option.mParamName.size();
		}
		offset = std::max<std::string::size_type>( offset, optionOffset );
	});
	if( anyShort ){
		offset += 4;
	}
	// Config
	fmt::print( "Options:\n" );
	app.config().options( [ offset, anyShort ](const Option & option){
		std::string::size_type paramsSize = 2 + 2 + option.mName.size();

		fmt::print( "  " );
		if( !option.mShortVersion.empty() ){
			fmt::print( "-{}, ", option.mShortVersion );
		}else if( anyShort ){
			fmt::print( "    " );
		}
		if( anyShort ){
			paramsSize += 4;
		}
		fmt::print( "--{}", option.mName );
		if( !option.mParamName.empty() ){
			fmt::print( " {}", option.mParamName );
			paramsSize += 1 + option.mParamName.size();
		}
		fmt::print("{:>{}}", "", 2 + offset - paramsSize );
		fmt::print( "{}", option.mDescription );
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
