#pragma once

#include <spdlog/fmt/fmt.h>
#include <iostream>
#include <fstream>
#include <set>
#include <string>

#include "App.h"
#include "Commander.h"
#include "Config.h"
#include "utilities/string.h"

namespace drea::core::integrations::Help {

//! First line of a description: the command list is one line per command, the
//! full text belongs to the help of the command itself
inline std::string summary( const std::string & description )
{
	return utilities::string::firstLine( description );
}

inline void version( const drea::core::App & app )
{
	fmt::print( "{} version {}\n", app.name(), app.version() );
}

inline void help( const drea::core::App & app, std::string_view commandName )
{
	if( app.commander().empty() ){
		fmt::print( "This app has no commands\n" );
	}else{
		if( commandName.empty() ){
			std::string::size_type offset = 0;
			bool anySubCmd = false;
			bool anyApp = false;
			bool anyCommon = false;

			app.commander().commands( [ &app, &offset, &anySubCmd, &anyApp, &anyCommon ](const Command & command ){
				if( !app.commander().isVisible( command ) ){
					return;
				}
				offset = std::max<std::string::size_type>( offset, command.mName.size() + 2 );
				if( !command.mSubcommand.empty() ){
					anySubCmd = true;
				}
				if( command.mParentCommand.empty() ){
					( command.mPredefined ? anyCommon : anyApp ) = true;
				}
			});
			if( anySubCmd ){
				offset += 8;
			}
			// application commands first, the predefined ones (completion, man)
			// under their own header
			auto printGroup = [ &app, offset ]( bool predefined ){
				app.commander().commands( [ &app, offset, predefined ](const Command & command ){
					if( !app.commander().isVisible( command ) || command.mPredefined != predefined ){
						return;
					}
					if( command.mParentCommand.empty() ){
						std::string::size_type cmdSize = 2 + command.mName.size();
						fmt::print( "  {}", command.mName );
						if( !command.mSubcommand.empty() ){
							fmt::print( " COMMAND" );
							cmdSize += 8;
						}
						fmt::print("{:>{}}", "", 2 + offset - cmdSize );
						fmt::print( "{}\n", summary( command.mDescription ) );
					}
				});
			};
			if( anyApp ){
				fmt::print( "Commands:\n" );
				printGroup( false );
			}
			if( anyCommon ){
				if( anyApp ){
					fmt::print( "\n" );
				}
				fmt::print( "Common commands:\n" );
				printGroup( true );
			}

			fmt::print( "\nUse \"{} COMMAND --help\" for more information about a command.\n", app.name() );
		}else{
			if( auto cmd = app.commander().find( commandName ); cmd && app.commander().isVisible( *cmd ) ){
				auto commands = utilities::string::split( commandName, "." );

				std::vector<std::string>	usage{ app.name(), utilities::string::join( commands, " " ) };

				if( !cmd->mSubcommand.empty() ){
					usage.emplace_back( "COMMAND" );
				}
				if( cmd->numberOfParams() > 0 || cmd->numberOfParams() == drea::core::Command::mUnlimitedParams ){
					usage.push_back( cmd->nameOfParamsForHelp() );
				}
				if( !cmd->mLocalParameters.empty() || !cmd->mGlobalParameters.empty() ){
					usage.emplace_back( "[OPTIONS]" );
				}
				fmt::print( "\nusage: {}\n\n", utilities::string::join( usage, " " ) );

				fmt::print( "{}{}\n", cmd->mDescription, cmd->mDeprecated ? " (deprecated)" : "" );
				if( !cmd->mParamChoices.empty() ){
					fmt::print( "\n{}: one of {}\n", cmd->mParamName, utilities::string::join( cmd->mParamChoices, ", " ) );
				}

				if( !cmd->mSubcommand.empty() ){
					std::string::size_type offset = 0;
					bool anySubCmd = false;

					for( const std::string & subCmdName: cmd->mSubcommand ){
						if( auto subCmd = app.commander().find( std::string( commandName ) + "." + subCmdName ); subCmd && app.commander().isVisible( *subCmd ) ){
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
						if( auto subCmd = app.commander().find( std::string( commandName ) + "." + subCmdName ); subCmd && app.commander().isVisible( *subCmd ) ){
							std::string::size_type cmdSize = 2 + subCmd->mName.size();

							fmt::print( "  {}", subCmd->mName );
							if( !subCmd->mSubcommand.empty() ){
								fmt::print( " COMMAND" );
								cmdSize += 8;
							}
							fmt::print("{:>{}}", "", 2 + offset - cmdSize );
							fmt::print( "{}\n", summary( subCmd->mDescription ) );
						}
					}
				}

				if( !cmd->mLocalParameters.empty() ){
					fmt::print( "\nOptions:\n");
					for( const std::string & arg: cmd->mLocalParameters ){
						if( auto config = app.config().find( arg ); config && config->helpInLine()  ){
							fmt::print( "  --{} {}\n", config->mName, config->mDescription );
						}
					}
				}
				if( !cmd->mGlobalParameters.empty() ){
					fmt::print( "\nGlobal options:\n");
					for( const std::string & arg: cmd->mGlobalParameters ){
						if( auto config = app.config().find( arg ); config && config->helpInLine() ){
							fmt::print( "  --{} {}\n", config->mName, config->mDescription );
						}
					}
				}
				if( !cmd->mExamples.empty() ){
					fmt::print( "\nExamples:\n" );
					for( const std::string & example: cmd->mExamples ){
						fmt::print( "  {}\n", example );
					}
				}
				if( !cmd->mSubcommand.empty() ){
					fmt::print( "\nUse \"{} {} COMMAND --help\" for more information about a command.\n", app.name(), utilities::string::join( commands, " " ));
				}
				if( auto footer = app.helpFooter( commandName ); !footer.empty() ){
					fmt::print( "\n{}\n", footer );
				}
			}
		}
	}
}

inline void helpOption( const drea::core::App & app, const Option & option, std::string::size_type offset, bool anyShort )
{
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
	if( option.mDeprecated ){
		fmt::print( " (deprecated)" );
	}
	if( !option.mChoices.empty() ){
		fmt::print( ". One of: {}", utilities::string::join( option.mChoices, ", " ) );
	}
	// the declared default and the resolved value are different facts: a
	// flag, env var or config file may have replaced the default by now
	if( const auto defaults = app.config().declaredDefault( option.mName ); !defaults.empty() ){
		if( option.mSensitive ){
			fmt::print( ". Default (hidden)" );
		}else{
			fmt::print( ". Default" );
			for( const auto & v: defaults ){
				fmt::print( " {}", option.toString( v ));
			}
		}
	}
	if( app.config().source( option.mName ) != "default" && !option.mValues.empty() ){
		if( option.mSensitive ){
			fmt::print( ". Current value (hidden)" );
		}else{
			fmt::print( ". Current value" );
			for( const auto & v: option.mValues ){
				fmt::print( " {}", option.toString( v ));
			}
		}
	}
	fmt::print( "\n" );
}

/*! The usage lines of the app: it may take arguments of its own (the root
	command), commands, or both. Only an app with commands of its own requires a
	COMMAND: with just the builtins (man, completion) a command is one option
	among others.
*/
inline std::string usageLines( const drea::core::App & app )
{
	auto 		root = app.commander().root();
	const bool	anyCommand = !app.commander().empty();
	std::string	res;

	if( root ){
		res = fmt::format( "usage: {} [OPTIONS]", app.name() );
		if( const std::string params = root->nameOfParamsForHelp(); !params.empty() ){
			res += " " + params;
		}
		res += "\n";
		if( anyCommand ){
			res += fmt::format( "       {} COMMAND [OPTIONS]\n", app.name() );
		}
	}else if( app.commander().hasAppCommands() ){
		res = fmt::format( "usage: {} COMMAND [OPTIONS]\n", app.name() );
	}else if( anyCommand ){
		res = fmt::format( "usage: {} [COMMAND] [OPTIONS]\n", app.name() );
	}else{
		res = fmt::format( "usage: {} [OPTIONS]\n", app.name() );
	}
	return res;
}

inline void help( const drea::core::App & app )
{
	std::string::size_type 	offset = 0;
	bool					anyShort = false;
	bool					anyLineApp = false;
	bool					anyLineCommon = false;
	bool					anyFile = false;

	// Options exclusively used as local-options on specific commands — hide from global help
	std::set<std::string> localOnlyOptions;
	{
		std::set<std::string> allLocalOptions;
		std::set<std::string> allGlobalOptions;

		app.commander().commands( [&allLocalOptions, &allGlobalOptions]( const Command & command ){
			for( const auto & opt: command.mLocalParameters ){
				allLocalOptions.insert( opt );
			}
			for( const auto & opt: command.mGlobalParameters ){
				allGlobalOptions.insert( opt );
			}
		});
		for( const auto & opt: allLocalOptions ){
			if( allGlobalOptions.find( opt ) == allGlobalOptions.end() ){
				localOnlyOptions.insert( opt );
			}
		}
	}

	fmt::print( "\n{}\n", app.description() );
	fmt::print( "{}", usageLines( app ) );
	{
		auto root = app.commander().root();

		if( root ){
			if( !root->mDescription.empty() ){
				fmt::print( "\n{}\n", root->mDescription );
			}
			if( !root->mParamChoices.empty() ){
				fmt::print( "\n{}: one of {}\n", root->mParamName, utilities::string::join( root->mParamChoices, ", " ) );
			}
			if( !root->mExamples.empty() ){
				fmt::print( "\nExamples:\n" );
				for( const std::string & example: root->mExamples ){
					fmt::print( "  {}\n", example );
				}
			}
		}
		fmt::print( "\n" );
	}

	app.config().options( [ &offset, &anyShort, &anyLineApp, &anyLineCommon, &anyFile, &localOnlyOptions ](const Option & option){
		if( localOnlyOptions.find( option.mName ) != localOnlyOptions.end() ){
			return;
		}
		std::string::size_type optionOffset = 2 + 2 + option.mName.size();
		if( !option.mShortVersion.empty() ){
			anyShort = true;
		}
		if( !option.mParamName.empty() ){
			optionOffset += 1 + option.mParamName.size();
		}
		if( option.helpInLine() ){
			( option.mPredefined ? anyLineCommon : anyLineApp ) = true;
		}
		if( option.helpInFileOnly() ){
			anyFile = true;
		}
		offset = std::max<std::string::size_type>( offset, optionOffset );
	});
	if( anyShort ){
		offset += 4;
	}
	// Config: application options first, the predefined drea set under its own header
	if( anyLineApp ){
		fmt::print( "Options:\n" );
		app.config().options( [ &app, offset, anyShort, &localOnlyOptions ](const Option & option){
			if( option.helpInLine() && !option.mPredefined && localOnlyOptions.find( option.mName ) == localOnlyOptions.end() ){
				helpOption( app, option, offset, anyShort );
			}
		});
	}
	if( anyLineCommon ){
		if( anyLineApp ){
			fmt::print( "\n" );
		}
		fmt::print( "Common options:\n" );
		app.config().options( [ &app, offset, anyShort, &localOnlyOptions ](const Option & option){
			if( option.helpInLine() && option.mPredefined && localOnlyOptions.find( option.mName ) == localOnlyOptions.end() ){
				helpOption( app, option, offset, anyShort );
			}
		});
	}
	if( anyFile ){
		fmt::print( "Config file options:\n" );
		app.config().options( [ &app, offset, anyShort, &localOnlyOptions ](const Option & option){
			if( option.helpInFileOnly() && localOnlyOptions.find( option.mName ) == localOnlyOptions.end() ){
				helpOption( app, option, offset, anyShort );
			}
		});
	}
	fmt::print( "\n" );

	// Commander
	help( app, {} );

	if( auto footer = app.helpFooter( {} ); !footer.empty() ){
		fmt::print( "\n{}\n", footer );
	}
}

}
