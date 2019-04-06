#pragma once

#include <spdlog/fmt/fmt.h>
#include <iostream>
#include <fstream>

#include "App.h"
#include "Commander.h"
#include "Config.h"

namespace drea { namespace core { namespace integrations { namespace Bash {

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

}}}}
