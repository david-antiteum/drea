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

namespace drea { namespace core {

class App;

/*! Configuration options of the application.

	Reads data from (in this order):
	- defaults
	- config file (if --config-file or Config::setDefaultConfigFile are used)
	- env variables (if the prefix is set, \see Config::setEnvPrefix)
	- external systems (if a remote provider is set, \see Config::addRemoteProvider)
	- command line flags
	- set values by the app (\see Config::set)
*/
class DREA_CORE_API Config
{
public:
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

	/*! Set the prefix for env variables for this app.

		For example, if there is a config option called verbose and an app sets the prefix to CAL, then 
		we will look for the variable CAL_verbose
	*/
	void setEnvPrefix( const std::string & value );

	/*! Add a remote provider (only consul for now) in a endpoint to read config settings (in YAML or JSON)
		stored in a key.
	*/
	void addRemoteProvider( const std::string & provider, const std::string & host, const std::string & key );

	/*! Any option?
	*/
	bool empty() const;

	/*! Access the options
	*/
	void options( std::function<void(const Option&)> f ) const;

	/*! Find an option by name. Return nullptr if not found
	*/
	jss::object_ptr<Option> find( const std::string & optionName ) const;

	/*! Returns true if the option has been set, either via flags, config files, sets... or because
		it has a default value.
	*/
	bool used( const std::string & optionName ) const;

	/*! Returns the number of times that an option appears. Use, for example, to increase the verbosity.
	*/
	unsigned int intensity( const std::string & optionName ) const;

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
		If the value cannot be converted, the method will throw.
	*/
	template<typename T>
	T get( const std::string & optionName ) const
	{
		auto option = find( optionName );

		if( option && !option->mValues.empty() ){
			return std::get<T>( option->mValues.front() );
		}
		return T{};
	}

	void reportUnknownArgument( const std::string & optionName ) const;

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

}}
