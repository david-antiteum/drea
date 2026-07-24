#pragma once

#include <spdlog/fmt/fmt.h>
#include <iostream>
#include <ostream>
#include <vector>

#include "App.h"
#include "Config.h"
#include "describe.h"
#include <drea/core/ExitCode.h>

namespace drea::core::integrations::Help {

/*! Runtime counterpart of describe: --describe emits the static command and
	option tree, --validate checks the configuration resolved from every
	source (defaults, remote sources, config file, environment, flags) and
	reports the problems. Human output goes to stderr, machine output
	(--json) to stdout. \see docs/configuration.md
*/

//! Exit code for a set of findings: Ok when empty; NoInput when a config
//! file cannot be read; ConfigError for structural problems (unknown keys,
//! missing required options, wrong scopes, gated commands, bad option
//! references); DataError when only values are wrong (parse errors, out of
//! range, bad choices, missing params). File beats structural beats values.
inline ExitCode validateExitCode( const std::vector<Config::Finding> & findings )
{
	if( findings.empty() ){
		return ExitCode::Ok;
	}
	bool	structural = false;

	for( const auto & finding: findings ){
		if( finding.mCode == "file_error" ){
			return ExitCode::NoInput;
		}
		structural = structural || finding.mCode == "unknown_key" || finding.mCode == "missing_required"
			|| finding.mCode == "wrong_scope" || finding.mCode == "disabled_group" || finding.mCode == "unknown_option_ref";
	}
	return structural ? ExitCode::ConfigError : ExitCode::DataError;
}

//! Report the findings and return the process exit code. Values of
//! sensitive options are already masked in the findings themselves.
inline int validateConfig( const App & app, bool json, std::ostream & out, std::ostream & err )
{
	namespace detail = describe_detail;

	const auto	findings = app.config().findings();

	if( json ){
		out << "{\n";
		out << "  \"schema\": \"drea-validate/1\",\n";
		out << fmt::format( "  \"app\": {},\n", detail::jsonQuoted( app.name() ) );
		out << fmt::format( "  \"version\": {},\n", detail::jsonQuoted( app.version() ) );
		out << fmt::format( "  \"valid\": {},\n", findings.empty() );
		out << "  \"findings\": [";
		bool	first = true;
		for( const auto & finding: findings ){
			out << fmt::format( "{}\n    {{\n", first ? "" : "," );
			first = false;
			out << fmt::format( "      \"option\": {},\n", detail::jsonQuoted( finding.mName ) );
			if( !finding.mSource.empty() ){
				out << fmt::format( "      \"source\": {},\n", detail::jsonQuoted( finding.mSource ) );
			}
			out << fmt::format( "      \"code\": {},\n", detail::jsonQuoted( finding.mCode ) );
			out << fmt::format( "      \"message\": {}\n", detail::jsonQuoted( finding.mMessage ) );
			out << "    }";
		}
		out << ( first ? "]\n" : "\n  ]\n" );
		out << "}\n";
	}else{
		if( findings.empty() ){
			err << fmt::format( "{}: the configuration is valid\n", app.name() );
		}else{
			err << fmt::format( "{}: the configuration has {} problem{}:\n", app.name(), findings.size(), findings.size() == 1 ? "" : "s" );
			for( const auto & finding: findings ){
				if( finding.mSource.empty() ){
					err << fmt::format( "  - {} [{}]\n", finding.mMessage, finding.mCode );
				}else{
					err << fmt::format( "  - {} [{}, from {}]\n", finding.mMessage, finding.mCode, finding.mSource );
				}
			}
		}
	}
	return toInt( validateExitCode( findings ) );
}

inline int validateConfig( const App & app, bool json )
{
	return validateConfig( app, json, std::cout, std::cerr );
}

}
