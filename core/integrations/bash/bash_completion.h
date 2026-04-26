#pragma once

#include <spdlog/fmt/fmt.h>
#include <iostream>
#include <fstream>
#include <ostream>
#include <list>
#include <set>


#include "App.h"
#include "Commander.h"
#include "Config.h"

namespace drea::core::integrations::Bash {

static std::list<std::string> calculateAutoCompletion( const drea::core::App & app )
{
	std::list<std::string>	res;

	if( app.commander().arguments().size() == 0 ){
		app.commander().commands( [&app, &res]( const Command & cmd ){
			if( cmd.mParentCommand.empty() && app.commander().isVisible( cmd ) ){
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
				if( !app.commander().isVisible( cmd ) ){
					return;
				}
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
static void generateAutoCompletion( const drea::core::App & app, std::ostream & out )
{
	out << "#!/usr/bin/env bash\n";
	out << "_" << app.name() << "()\n";
	out << "{\n";
	out << "    local cur prev opts base\n";
	out << "    COMPREPLY=()\n";
	out << "    cur=\"${COMP_WORDS[COMP_CWORD]}\"\n";
	out << "    prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n";
	out << "    opts=\"";
	app.commander().commands( [&app, &out]( const Command & cmd ){
		if( app.commander().isVisible( cmd ) && cmd.mParentCommand.empty() ){
			out << " " << cmd.mName;
		}
	});
	out << "\"\n";

	out << "    case \"${prev}\" in\n";
	app.commander().commands( [&app, &out]( const Command & cmd ){
		if( !app.commander().isVisible( cmd ) ){
			return;
		}
		out << "        " << cmd.mName << ")\n";
		out << "            COMPREPLY=( $(compgen -W \"";
		for( const auto & sub: cmd.mSubcommand ){
			out << " " << sub;
		}
		for( const auto & str: cmd.mLocalParameters ){
			out << " --" << str;
		}
		for( const auto & str: cmd.mGlobalParameters ){
			out << " --" << str;
		}
		out << "\" -- ${cur}) )\n";
		out << "            return 0\n";
		out << "            ;;\n";
	});
	out << "        *)\n";
	out << "        ;;\n";
	out << "    esac\n";

	out << "    COMPREPLY=($(compgen -W \"${opts}\" -- ${cur}))\n";
	out << "    return 0\n";
	out << "}\n";
	out << "complete -F _" << app.name() << " " << app.name() << "\n";
}

[[maybe_unused]] static void generateAutoCompletion( const drea::core::App & app )
{
	std::ofstream completionFile;

	completionFile.open( fmt::format( "{}-completion.sh", app.name() ).c_str(), std::ios::out );

	if( completionFile.is_open() ){
		generateAutoCompletion( app, completionFile );
		completionFile.close();
	}
}

}
