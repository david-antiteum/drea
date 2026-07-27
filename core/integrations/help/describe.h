#pragma once

#include <spdlog/fmt/fmt.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "App.h"
#include "Commander.h"
#include "Config.h"
#include "utilities/string.h"

namespace drea::core::integrations::Help {

/*! Machine readable description of the app: commands (with sub commands),
	options and their limits, as a single JSON object. Aimed at AI agents
	and other tools that want to discover the CLI without scraping --help.

	Hand written emitter so that the feature does not depend on ENABLE_JSON.
*/
namespace describe_detail {

inline std::string escaped( std::string_view text )
{
	std::string	res;

	res.reserve( text.size() );
	for( const char c: text ){
		switch( c ){
			case '"':	res += "\\\"";	break;
			case '\\':	res += "\\\\";	break;
			case '\n':	res += "\\n";	break;
			case '\r':	res += "\\r";	break;
			case '\t':	res += "\\t";	break;
			case '\b':	res += "\\b";	break;
			case '\f':	res += "\\f";	break;
			default:
				if( static_cast<unsigned char>( c ) < 0x20 ){
					res += fmt::format( "\\u{:04x}", static_cast<unsigned char>( c ) );
				}else{
					res += c;
				}
		}
	}
	return res;
}

inline std::string jsonQuoted( std::string_view text )
{
	return fmt::format( "\"{}\"", escaped( text ) );
}

inline std::string typeName( const Option & option )
{
	return option.typeName();
}

inline std::string valueLiteral( const Option & option, const OptionValue & value )
{
	if( std::holds_alternative<bool>( value ) ){
		return std::get<bool>( value ) ? "true" : "false";
	}else if( std::holds_alternative<int>( value ) ){
		return fmt::format( "{}", std::get<int>( value ) );
	}else if( std::holds_alternative<double>( value ) ){
		const double	number = std::get<double>( value );

		// JSON has no nan or inf. fromString refuses them, but an app may put
		// one straight into Option::mValues, and invalid JSON is worse than a
		// quoted oddity
		if( !std::isfinite( number ) ){
			return jsonQuoted( fmt::format( "{}", number ) );
		}
		return fmt::format( "{}", number );
	}else if( std::holds_alternative<std::string>( value ) ){
		return jsonQuoted( std::get<std::string>( value ) );
	}
	return jsonQuoted( option.toString( value ) );
}

inline std::string scopeName( const Option & option )
{
	return option.scopeName();
}

//! Same mapping as the env lookup in Config: chars invalid in shell
//! variable names become '_'
inline std::string envVarName( const std::string & prefix, const std::string & name )
{
	std::string	sanitized = name;

	for( char & c: sanitized ){
		if( !std::isalnum( static_cast<unsigned char>( c ) ) && c != '_' ){
			c = '_';
		}
	}
	return prefix + "_" + sanitized;
}

}

inline void describe( const drea::core::App & app, std::ostream & os )
{
	namespace detail = describe_detail;

	os << "{\n";
	os << fmt::format( "  \"schema\": \"drea-describe/1\",\n" );
	os << fmt::format( "  \"app\": {},\n", detail::jsonQuoted( app.name() ) );
	os << fmt::format( "  \"version\": {},\n", detail::jsonQuoted( app.version() ) );
	os << fmt::format( "  \"description\": {},\n", detail::jsonQuoted( app.description() ) );
	{
		// the forms the app accepts: root params, commands, or both
		std::vector<std::string>	usages;

		if( auto root = app.commander().root() ){
			usages.push_back( fmt::format( "{} [OPTIONS] {}", app.name(), root->nameOfParamsForHelp() ) );
		}
		if( !app.commander().empty() ){
			usages.push_back( fmt::format( "{} COMMAND [SUBCOMMAND ...] [PARAMS] [OPTIONS]", app.name() ) );
		}
		if( usages.empty() ){
			usages.push_back( fmt::format( "{} [OPTIONS]", app.name() ) );
		}
		os << fmt::format( "  \"usage\": {},\n", detail::jsonQuoted( utilities::string::join( usages, " | " ) ) );
	}
	os << "  \"conventions\": {\n";
	os << "    \"option-syntax\": \"pass options as --name value or --name=value; an option with a short version also accepts -x\",\n";
	os << "    \"bool-options\": \"bool options are flags: --name enables, --no-name disables, unless the option is marked negatable false because it is an action (--help, --version, --validate)\",\n";
	os << "    \"option-types\": [\"bool\", \"int\", \"double\", \"string\"],\n";
	os << "    \"option-scopes\": [\"both\", \"command-line\", \"config-file\", \"none\"],\n";
	os << "    \"option-fields\": \"scope tells where an option may be set; min and max bound numeric values; choices is the closed set of legal values; nb-params is the fixed number of values the option takes per use (commands instead declare a min-params/max-params range for their positional params)\",\n";
	os << "    \"values\": \"every option value is a list: default is always an array, and any value-taking option may be repeated with values accumulating, so a scalar-looking option passed twice carries two elements. The app reads the first element when it expects a single value (first wins). Repeating a flag increases its intensity (-vv). The default of a sensitive option is masked as the string [redacted]\",\n";
	os << "    \"required-options\": \"required means the option must end up with a value from any source; a default already satisfies it\",\n";
	os << "    \"scopes\": \"both = command line plus config sources; command-line = flags only; config-file = config sources only, which bundle remote sources, the config file and environment variables; none = not set by users, the app sets it in code (listed so its meaning is known). An option reads the environment only when its scope permits config sources AND it carries an env field\",\n";
	os << "    \"env-derivation\": \"an option is read from the variable env-prefix + '_' + the option name with every character outside [A-Za-z0-9_] replaced by '_'; the all-uppercase spelling is also accepted. The env field gives the exact name per option\",\n";
	os << "    \"command-options\": \"a command accepts its local-options and global-options; global-options are also accepted by its subcommands\",\n";
	os << "    \"command-params\": \"param-choices, when present, is the closed set of legal values of a command's single positional param; absent means the values are unrestricted\",\n";
	os << "    \"command-groups\": \"a command listing groups is only available when one of those groups is enabled by the app; commands gated by disabled groups are omitted from this description\",\n";
	os << "    \"root-params\": \"root, when present, describes the positional arguments the app accepts with no command given; a leading -- forces the remaining arguments to be root params\",\n";
	os << "    \"config-precedence\": \"defaults, then remote config sources, then the config file, then environment variables, then command line flags; later sources win\"\n";
	os << "  },\n";
	if( !app.config().envPrefix().empty() ){
		os << fmt::format( "  \"env-prefix\": {},\n", detail::jsonQuoted( app.config().envPrefix() ) );
	}

	os << "  \"options\": [";
	bool firstOption = true;
	app.config().options( [ &app, &os, &firstOption ]( const Option & option ){
		os << fmt::format( "{}\n    {{\n", firstOption ? "" : "," );
		firstOption = false;
		os << fmt::format( "      \"name\": {},\n", detail::jsonQuoted( option.mName ) );
		if( !option.mShortVersion.empty() ){
			os << fmt::format( "      \"short\": {},\n", detail::jsonQuoted( option.mShortVersion ) );
		}
		os << fmt::format( "      \"description\": {},\n", detail::jsonQuoted( option.mDescription ) );
		os << fmt::format( "      \"type\": {},\n", detail::jsonQuoted( detail::typeName( option ) ) );
		if( !option.mParamName.empty() ){
			os << fmt::format( "      \"param-name\": {},\n", detail::jsonQuoted( option.mParamName ) );
		}
		if( option.mNbParams == Option::mUnlimitedParams ){
			os << "      \"nb-params\": \"unlimited\",\n";
		}else{
			os << fmt::format( "      \"nb-params\": {},\n", option.numberOfParams() );
		}
		// a non finite bound is omitted rather than printed: it cannot act (see
		// the bad_definition finding) and nan is not a JSON number
		if( option.mMin && std::isfinite( *option.mMin ) ){
			os << fmt::format( "      \"min\": {},\n", *option.mMin );
		}
		if( option.mMax && std::isfinite( *option.mMax ) ){
			os << fmt::format( "      \"max\": {},\n", *option.mMax );
		}
		if( !option.mChoices.empty() ){
			os << "      \"choices\": [";
			bool firstChoice = true;
			for( const auto & choice: option.mChoices ){
				os << fmt::format( "{}{}", firstChoice ? "" : ", ", detail::jsonQuoted( choice ) );
				firstChoice = false;
			}
			os << "],\n";
		}
		// the declared default, not the resolved value: sources (flags, env,
		// config file) may have replaced mValues by the time describe runs
		if( const auto defaults = app.config().declaredDefault( option.mName ); !defaults.empty() ){
			if( option.mSensitive ){
				os << "      \"default\": \"[redacted]\",\n";
			}else{
				os << "      \"default\": [";
				bool firstValue = true;
				for( const auto & value: defaults ){
					os << fmt::format( "{}{}", firstValue ? "" : ", ", detail::valueLiteral( option, value ) );
					firstValue = false;
				}
				os << "],\n";
			}
		}
		if( !app.config().envPrefix().empty() && ( option.mScope == Option::Scope::Both || option.mScope == Option::Scope::File ) ){
			os << fmt::format( "      \"env\": {},\n", detail::jsonQuoted( detail::envVarName( app.config().envPrefix(), option.mName ) ) );
		}
		os << fmt::format( "      \"scope\": {},\n", detail::jsonQuoted( detail::scopeName( option ) ) );
		if( option.mDeprecated ){
			os << "      \"deprecated\": true,\n";
		}
		// only for bool options, where --no-<name> is otherwise available
		if( option.mType == typeid( bool ) && !option.mNegatable ){
			os << "      \"negatable\": false,\n";
		}
		os << fmt::format( "      \"required\": {},\n", option.mRequired );
		os << fmt::format( "      \"sensitive\": {}\n", option.mSensitive );
		os << "    }";
	});
	os << "\n  ],\n";

	// Commands as a tree: each command carries its visible subcommands as
	// nested objects, mirroring both the YAML definition and the command line
	std::function<void( const Command &, const std::string &, const std::string &, bool )> emitCommand;

	emitCommand = [ & ]( const Command & command, const std::string & fullName, const std::string & pad, bool first ){
		os << fmt::format( "{}\n{}{{\n", first ? "" : ",", pad );
		os << fmt::format( "{}  \"name\": {},\n", pad, detail::jsonQuoted( command.mName ) );
		os << fmt::format( "{}  \"description\": {},\n", pad, detail::jsonQuoted( command.mDescription ) );
		if( !command.mParamName.empty() ){
			os << fmt::format( "{}  \"params-names\": {},\n", pad, detail::jsonQuoted( command.mParamName ) );
		}
		// minParams() reflects mUnlimitedParams (negative as int) when the
		// command takes unlimited params: no minimum then, report 0
		os << fmt::format( "{}  \"min-params\": {},\n", pad, std::max( command.minParams(), 0 ) );
		if( command.maxParams() == Command::mUnlimitedParams ){
			os << fmt::format( "{}  \"max-params\": \"unlimited\",\n", pad );
		}else{
			os << fmt::format( "{}  \"max-params\": {},\n", pad, command.maxParams() );
		}
		auto stringList = [ &os, &pad ]( const char * key, const std::vector<std::string> & values, bool last ){
			os << fmt::format( "{}  \"{}\": [", pad, key );
			bool firstValue = true;
			for( const auto & value: values ){
				os << fmt::format( "{}{}", firstValue ? "" : ", ", detail::jsonQuoted( value ) );
				firstValue = false;
			}
			os << ( last ? "]" : "],\n" );
		};
		if( command.mDeprecated ){
			os << fmt::format( "{}  \"deprecated\": true,\n", pad );
		}
		if( !command.mParamChoices.empty() ){
			stringList( "param-choices", command.mParamChoices, false );
		}
		stringList( "local-options", command.mLocalParameters, false );
		if( !command.mGroups.empty() ){
			stringList( "global-options", command.mGlobalParameters, false );
			stringList( "groups", command.mGroups, command.mExamples.empty() );
		}else{
			stringList( "global-options", command.mGlobalParameters, command.mExamples.empty() );
		}
		if( !command.mExamples.empty() ){
			stringList( "examples", command.mExamples, true );
		}
		bool firstSub = true;
		for( const std::string & subName: command.mSubcommand ){
			if( auto sub = app.commander().find( fullName + "." + subName ); sub && app.commander().isVisible( *sub ) ){
				if( firstSub ){
					os << fmt::format( ",\n{}  \"commands\": [", pad );
				}
				emitCommand( *sub, fullName + "." + subName, pad + "    ", firstSub );
				firstSub = false;
			}
		}
		if( !firstSub ){
			os << fmt::format( "\n{}  ]", pad );
		}
		os << fmt::format( "\n{}}}", pad );
	};

	// the positional arguments accepted with no command given
	if( auto root = app.commander().root() ){
		auto rootList = [ &os ]( const char * key, const std::vector<std::string> & values ){
			os << fmt::format( "    \"{}\": [", key );
			bool firstValue = true;
			for( const auto & value: values ){
				os << fmt::format( "{}{}", firstValue ? "" : ", ", detail::jsonQuoted( value ) );
				firstValue = false;
			}
			os << "],\n";
		};

		os << "  \"root\": {\n";
		if( !root->mDescription.empty() ){
			os << fmt::format( "    \"description\": {},\n", detail::jsonQuoted( root->mDescription ) );
		}
		os << fmt::format( "    \"params-names\": {},\n", detail::jsonQuoted( root->mParamName ) );
		if( !root->mParamChoices.empty() ){
			rootList( "param-choices", root->mParamChoices );
		}
		if( !root->mExamples.empty() ){
			rootList( "examples", root->mExamples );
		}
		os << fmt::format( "    \"min-params\": {},\n", std::max( root->minParams(), 0 ) );
		if( root->maxParams() == Command::mUnlimitedParams ){
			os << "    \"max-params\": \"unlimited\"\n";
		}else{
			os << fmt::format( "    \"max-params\": {}\n", root->maxParams() );
		}
		os << "  },\n";
	}

	os << "  \"commands\": [";
	bool firstCommand = true;
	app.commander().commands( [ & ]( const Command & command ){
		if( !command.mParentCommand.empty() || !app.commander().isVisible( command ) ){
			return;
		}
		emitCommand( command, command.mName, "    ", firstCommand );
		firstCommand = false;
	});
	os << "\n  ]\n";
	os << "}\n";
}

}
