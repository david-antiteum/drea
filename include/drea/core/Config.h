#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>
#include <functional>
#include <object_ptr/object_ptr.hpp>

#include "Export.h"
#include "Option.h"

namespace spdlog {
	class logger;
}

namespace drea::log {
	class Logger;
}

namespace drea::core {

class App;

/*! Configuration options of the application.

	Reads data from (in this order):
	- defaults
	- remote config sources passed on the command line as --config-source <uri>.
	  Supported URI schemes: aws://<region>/<secret-id> (requires ENABLE_AWS).
	- config file (if --config-file or Config::setDefaultConfigFile are used)
	- env variables (if the prefix is set, \see Config::setEnvPrefix)
	- command line flags
	- set values by the app (\see Config::set)
*/
class DREA_CORE_API Config
{
public:
	/*! One problem found while checking the resolved configuration. Produced
		while sources are applied (parse errors, unknown keys, unreadable
		config files) and by the declarative checks in Config::findings.
	*/
	struct Finding
	{
		std::string		mName;		//!< option name, config key or dotted command name
		std::string		mSource;	//!< source of the offending value: "default", "config-source", "config-file", "environment" or "flag"; empty when no single source applies
		std::string		mCode;		//!< stable machine code: "parse_error", "file_error", "unknown_key", "missing_required", "bad_choice", "out_of_range", "missing_params", "wrong_scope", "unknown_option_ref", "disabled_group", "bad_source" or "bad_definition"
		std::string		mMessage;	//!< human readable message. Values of sensitive options are masked as [redacted]
	};

	explicit Config( App & app );
	~Config();

	/*! Add a default value for the config-file entry
		This is a special value that must but set (if required) before parsing the commands
	*/
	void setDefaultConfigFile( const std::string & filePath );

	/*! Adds defaults options to the app
	*/
	Config & addDefaults();

	/*! Adds an option to the app
	*/
	void add( const Option & option );

	/*! Adds an options to the app
	*/
	void add( const std::vector<Option> & options );

	/*! Remove an option by name. Useful to drop a default that the app does
		not want to expose (for example "graylog-host" when the app does not
		support graylog logging, or "config-source" when it does not load
		remote configuration).

		The option is erased from the registry and from the "used" flag list.
		Subsequent CLI flags referring to the removed option will be reported
		as unknown.
	*/
	void remove( std::string_view optionName );

	/*! Set the prefix for env variables for this app.

		For example, if there is a config option called verbose and an app sets the prefix to CAL, then 
		we will look for the variable CAL_verbose
	*/
	void setEnvPrefix( const std::string & value );

	/*! The prefix for env variables for this app. Empty if not set.
	*/
	[[nodiscard]] const std::string & envPrefix() const;

	/*! Any option?
	*/
	[[nodiscard]] bool empty() const;

	/*! Access the options
	*/
	void options( std::function<void(const Option&)> f ) const;

	/*! Find an option by name. Return nullptr if not found
	*/
	[[nodiscard]] jss::object_ptr<Option> find( std::string_view optionName ) const;

	/*! Whether some source supplied the option: a command line flag, a config
		file, a remote source, the environment or a Config::set call. A declared
		default does not count, which is what makes this the way to tell
		`--equal 0` from no `--equal` at all.

		This is a presence question. For a bool option ask for the value
		instead, with get<bool>(): a config file or environment variable may
		carry "false", which counts as supplied but means off.
	*/
	[[nodiscard]] bool used( const std::string & optionName ) const;

	/*! Returns the number of times that an option appears. Use, for example, to increase the verbosity.
	*/
	[[nodiscard]] unsigned int intensity( const std::string & optionName ) const;

	/*! 
	*/
	void registerUse( const std::string & optionName );

	/*! Set the value of an option from a string. The value will be converted to the declared type.
		If the value cannot be converted, the method will report an error and exit.
	*/
	void set( const std::string & optionName, const std::string & value );

	/*! Append the value of an option from a string. The value will be converted to the declared type.
		If the value cannot be converted, the method will report an error and exit.
	*/
	void append( const std::string & optionName, const std::string & value );

	/*! Read the value of an option using a give type.
		If there is more than one value, returns the first one.
		If the value cannot be converted, the method will throw.
	*/
	template<typename T>
	[[nodiscard]] T get( std::string_view optionName ) const
	{
		if( auto option = find( optionName ); option && !option->mValues.empty() ){
			return std::get<T>( option->mValues.front() );
		}
		return T{};
	}

	/*! Read all the values of an option using a give type.
		If any of the values cannot be converted, the method will throw.
	*/
	template<typename T>
	[[nodiscard]] std::vector<T> getAll( std::string_view optionName ) const
	{
		if( auto option = find( optionName ); option && !option->mValues.empty() ){
			std::vector<T>	res;

			for( const OptionValue & optionValue: option->mValues ){
				res.push_back( std::get<T>( optionValue ) );
			}
			return res;
		}
		return std::vector<T>{};
	}

	/*! Check the declarative constraints of all options (`required`, `min`,
		`max` in commands.yml) against their resolved values. Returns one
		message per violation; empty means the config is valid.

		App::parse runs this after source resolution and exits with
		ExitCode::ConfigError on failure (unless --help or --version was
		requested).
	*/
	[[nodiscard]] std::vector<std::string> validate() const;

	/*! Check the resolved configuration and return every problem found, not
		just the first one: the issues collected while sources were applied
		(values that do not parse as the declared type, unknown config keys,
		unreadable or unparseable config files) plus the declarative checks
		(`required`, `min`/`max`, `choices`, `nb-params`, `scope`) and the
		command gated by disabled groups, if one was requested.

		This is the model behind --validate, which reports the findings and
		exits (\see docs/configuration.md for the codes and exit codes).
		Config::validate is the fatal subset that App::parse enforces.
	*/
	[[nodiscard]] std::vector<Finding> findings() const;

	/*! The default values an option declared, regardless of what source
		resolution later put in its values (Config::configure snapshots
		them; before it runs, the current values are the declared defaults).
		Empty if the option declares no default.
	*/
	[[nodiscard]] std::vector<OptionValue> declaredDefault( std::string_view optionName ) const;

	/*! Which source provided the current value of an option: "default",
		"config-source", "config-file", "environment", "flag" or "code"
		(Config::set after parsing).
	*/
	[[nodiscard]] std::string source( std::string_view optionName ) const;

	/*! True when a real source (flag, environment, config file, remote
		source, code) set the option to exactly its declared default: the
		setting is redundant. False for values that only come from the
		default, differ from it, or have no declared default to match.
	*/
	[[nodiscard]] bool redundant( std::string_view optionName ) const;

	/*! Emit one info line per option with its resolved value and source,
		flagging redundant settings (\see Config::redundant). Sensitive
		values are redacted (unless --no-log-redact). Options that were
		never set are skipped.

		App::parse calls this automatically when --log-effective-config is on.
	*/
	void logEffective( drea::log::Logger & logger ) const;

	void reportUnknownArgument( const std::string & optionName ) const;

	/*! Whether the option may take a value from the source being read right
		now — a config file, a remote source or the environment. Options scoped
		to the command line (`line`), or to no user source at all (`none`),
		may not: the call then reports a `wrong_scope` finding and the caller
		must leave the option untouched, neither used nor set.

		The scope rules live here so that every reader applies them the same
		way. Called by the config readers and the environment scan.
	*/
	[[nodiscard]] bool acceptsCurrentSource( const std::string & optionName );

	// Methods called by App

	/*! Init the system with arguments and apply values in order.
		Don't call this method directly. App::parse will do it.
	*/
	void configure( const std::vector<std::string> & args );

	/*! Setup and create a logger based on the config
		Don't call this method directly. App::parse will do it.
	*/
	std::shared_ptr<spdlog::logger> setupLogger() const;

private:
	struct Private;
	std::unique_ptr<Private>	d;
};

}
