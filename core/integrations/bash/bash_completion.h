#pragma once

#include <spdlog/fmt/fmt.h>
#include <iostream>
#include <fstream>
#include <ostream>

#include "App.h"
#include "Commander.h"
#include "Config.h"

namespace drea::core::integrations::Bash {

// as seem in https://debian-administration.org/article/317/An_introduction_to_bash_completion_part_2
inline void generateAutoCompletion( const drea::core::App & app, std::ostream & out )
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
	// options with a closed value set (choices) complete their values
	app.config().options( [&out]( const Option & option ){
		if( option.mChoices.empty() ){
			return;
		}
		out << "        --" << option.mName;
		if( !option.mShortVersion.empty() ){
			out << "|-" << option.mShortVersion;
		}
		out << ")\n";
		out << "            COMPREPLY=( $(compgen -W \"";
		for( const auto & choice: option.mChoices ){
			out << " " << choice;
		}
		out << "\" -- ${cur}) )\n";
		out << "            return 0\n";
		out << "            ;;\n";
	});
	app.commander().commands( [&app, &out]( const Command & cmd ){
		if( !app.commander().isVisible( cmd ) ){
			return;
		}
		out << "        " << cmd.mName << ")\n";
		out << "            COMPREPLY=( $(compgen -W \"";
		for( const auto & sub: cmd.mSubcommand ){
			out << " " << sub;
		}
		for( const auto & choice: cmd.mParamChoices ){
			out << " " << choice;
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

inline void generateAutoCompletion( const drea::core::App & app )
{
	std::ofstream completionFile;

	completionFile.open( fmt::format( "{}-completion.sh", app.name() ).c_str(), std::ios::out );

	if( completionFile.is_open() ){
		generateAutoCompletion( app, completionFile );
		completionFile.close();
	}
}

}
