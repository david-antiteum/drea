#pragma once

#include <spdlog/fmt/fmt.h>
#include <iostream>
#include <fstream>
#include <list>


#include "App.h"
#include "Commander.h"
#include "Config.h"

namespace drea::core::integrations::Bash {

static std::list<std::string> calculateAutoCompletion( const drea::core::App & app )
{
	std::list<std::string>	res;

	if( app.commander().arguments().size() == 0 ){
		app.commander().commands( [&res]( const Command & cmd ){
			if( cmd.mParentCommand.empty() ){
				res.push_back( cmd.mName );
			}
		});
	}else if( app.commander().arguments().size() == 1 ){
		if( app.commander().arguments().at(0) == "--" ){
			app.config().options( [ &res ](const Option & option){
				res.push_back( option.mName );
			});
		}else{
			std::string				exactCommand;
			std::list<std::string>	possibleCommand;

			app.commander().commands( [&exactCommand, &possibleCommand, &app]( const Command & cmd ){
				if( app.commander().arguments().at(0) == cmd.mName ){
					exactCommand = cmd.mName;
				}else if( cmd.mName.find( app.commander().arguments().at(0), 0 ) != std::string::npos ){
					possibleCommand.push_back( cmd.mName );
				}
			});
			if( exactCommand.empty() ){
				for( auto v: possibleCommand ){
					res.push_back( v );
				}
			}else{
				auto cmd = app.commander().find( exactCommand );

				if( cmd ){
					for( auto v: cmd->mSubcommand ){
						res.push_back( v );
					}
				}
			}
		}
	}
	return res;
}

// as seem in https://debian-administration.org/article/317/An_introduction_to_bash_completion_part_2
static void generateAutoCompletion( const drea::core::App & app )
{
	std::ofstream completionFile;
	
	completionFile.open( fmt::format( "{}-completion.sh", app.name() ).c_str(), std::ios::out );

	if( completionFile.is_open() ){
		completionFile << "#!/bin/bash\n";
		completionFile << "_" << app.name() << "()\n";
		completionFile << "{\n";
		completionFile << "    local cur prev opts base\n";
		completionFile << "    COMPREPLY=()\n";
		completionFile << "    cur=\"${COMP_WORDS[COMP_CWORD]}\"\n";
		completionFile << "    prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n";
		completionFile << "    opts=\"";
		app.commander().commands( [&completionFile]( const Command & cmd ){
			completionFile << " " << cmd.mName;
		});
		completionFile << "\"\n";

		completionFile << "    case \"${prev}\" in\n";
		app.commander().commands( [&completionFile]( const Command & cmd ){
			completionFile << "        " << cmd.mName << ")\n";
			completionFile << "            COMPREPLY=( $(compgen -W \"";
			for( auto str: cmd.mLocalParameters ){
				completionFile << " --" << str;
			}
			for( auto str: cmd.mGlobalParameters ){
				completionFile << " --" << str;
			}
			completionFile << "\" -- ${cur}) )\n";
			completionFile << "            return 0\n";
			completionFile << "            ;;\n";
		});
		completionFile << "        *)\n";
		completionFile << "        ;;\n";
		completionFile << "    esac\n";

		completionFile << "    COMPREPLY=($(compgen -W \"${opts}\" -- ${cur}))\n";
		completionFile << "    return 0\n";
		completionFile << "}\n";
		completionFile << "complete -F _" << app.name() << " " << app.name() << "\n";

		completionFile.close();
	}
}

}
