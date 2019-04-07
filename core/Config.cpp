
#include <vector>
#include <optional>
#include <iostream>
#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/fmt/fmt.h>
#include <woorm/levenshtein.h>

#include <stdlib.h>
#include <iostream>
#include <fstream>

#include "integrations/yaml/yaml_reader.h"
#include "integrations/json/json_reader.h"

#include "Config.h"
#include "App.h"

#include "integrations/graylog/graylog_sink.h"
#include "integrations/consul/kv_store.h"
#include "integrations/etcd/kv_store.h"

namespace drea { namespace core {

static std::string getenv( const std::string & prefix, const std::string & name )
{
	std::string		res;
	char			*env_p = nullptr;
#ifdef WIN32			
	size_t 	sz = 0;
	if( _dupenv_s( &env_p, &sz, (prefix + "_" + name).c_str() ) == 0 && env_p ){
		res = env_p;
	}
	free( env_p );
#else
	env_p = ::getenv( (prefix + "_" + name).c_str() );
	if( env_p ){
		res = env_p;
	}
#endif
	return res;
}

struct RemoteProvider
{
	std::string		mProvider;
	std::string		mHost;
	std::string		mKey;
};

}}

struct drea::core::Config::Private
{
	std::vector<std::string>				mFlags;
	std::vector<std::unique_ptr<Option>>	mOptions;
	std::string								mEnvPrefix;
	std::vector<RemoteProvider>				mRemoteProviders;

	jss::object_ptr<Option> find( const std::string & optionName ){
		jss::object_ptr<Option>	res;

		for( const auto & opt: mOptions ){
			if( opt->mName == optionName ){
				res = opt;
				break;
			}
		}
		return res;
	}

	void set( const std::string & optionName, const std::string & value )
	{
		if( auto option = find( optionName ) ){
			option->mValues.clear();
			append( optionName, value );
		}
	}

	void append( const std::string & optionName, const std::string & value )
	{
		if( auto option = find( optionName ) ){
			OptionValue	val = option->fromString( value );

			if( val.index() > 0 ){
				option->mValues.push_back( val );
			}else{
				exit( -1 );
			}
		}
	}

	bool readConfig( const std::string & val )
	{
		bool		res = true;

		if( drea::core::integration::yaml::valid( val ) ){
			drea::core::integration::yaml::readConfig( App::instance(), val );
		}else if( drea::core::integration::json::valid( val ) ){
			drea::core::integration::json::readConfig( App::instance(), val );
		}else{
			res = false;
		}
		return res;
	}

	std::string readFile( const std::string & configFileName )
	{
		// TODO ... use  std::filesystem::exists( configFileName )
		std::string		res;
		std::ifstream 	configFile;

		configFile.open( configFileName.c_str(), std::ios::in );
		if( configFile.is_open() ){
			std::stringstream buffer;
			buffer << configFile.rdbuf();
			res = buffer.str();
			if( res.empty() ){
				spdlog::warn( "The config file {} is empty", configFileName );
			}
		}else{
			spdlog::error( "Cannot read the config file {}", configFileName );
		}
		return res;
	}

	void readConfig( int argc, char * argv[] )
	{
		std::string					configFileName;

		for( int i = 1; i < argc-1; i++ ){
			if( std::string( argv[i] ) == "--config-file" ){
				auto fileData = readFile( argv[i+1] );
				if( !fileData.empty() ){
					if( readConfig( fileData ) == false ){
						spdlog::error( "Cannot determine the format of the config file {}", argv[i+1] );
					}
				}
				break;
			}
		}
	}
};

drea::core::Config::Config()
{
	d = std::make_unique<Private>();
}

drea::core::Config::~Config()
{
}

void drea::core::Config::addDefaults()
{
	add(
		{
			"verbose"
		}
	);
	add(
		{
			"help"
		}
	);
	add(
		{
			"version"
		}
	);
	add(
		{
			"log-file", "file name", "log messages to the file <file name>", {}, typeid( std::string )
		}
	);
	add(
		{
			"config-file", "file name", "read configs from file <file name>", {}, typeid( std::string )
		}
	);
	add(
		{
			"graylog-host", "schema://host:port", "Send logs to a graylog server. Example: http://localhost:12201", {}, typeid( std::string )
		}
	);
	add(
		{
			"generate-auto-completion"
		}
	);
}

bool drea::core::Config::empty() const
{
	return d->mOptions.empty();
}

void drea::core::Config::options( std::function<void(const drea::core::Option&)> f ) const
{
	for( const auto & opt: d->mOptions ){
		f( *opt );
	}
}

void drea::core::Config::add( const drea::core::Option & option )
{
	d->mOptions.push_back( std::make_unique<Option>( option ));
}

void drea::core::Config::setEnvPrefix( const std::string & value )
{
	d->mEnvPrefix = value;
}

void drea::core::Config::addRemoteProvider( const std::string & provider, const std::string & host, const std::string & key )
{
	d->mRemoteProviders.push_back( { provider, host, key } );
}

jss::object_ptr<drea::core::Option> drea::core::Config::find( const std::string & optionName ) const
{
	return d->find( optionName );
}

std::vector<std::string> drea::core::Config::configure( int argc, char * argv[] )
{
	// Order (less to more)
	// - defaults
	// - config file
	// - env variables
	// - external systems (as Consul)
	// - command line flags

	// Add values with defaults
	for( const auto & option: d->mOptions ){
		if( !option->mValues.empty() ){
			registerUse( option->mName );
		}
	}

	// Read the config file
	d->readConfig( argc, argv );

	// Env vars
	if( !d->mEnvPrefix.empty() ){
		for( const auto & option: d->mOptions ){
			std::string		env = drea::core::getenv( d->mEnvPrefix, option->mName );
			if( !env.empty() ){
				registerUse( option->mName );
				if( !option->mParamName.empty() ){
					set( option->mName, env );
				}
			}
		}
	}

	for( const RemoteProvider & provider: d->mRemoteProviders ){
		if( provider.mProvider == "consul" ){
			d->readConfig( integrations::Consul::KVStore( provider.mHost ).get( provider.mKey ) );
		}else if( provider.mProvider == "etcd" ){
			d->readConfig( integrations::etcd::KVStore( provider.mHost ).get( provider.mKey ) );
		}
	}

	std::vector<std::string>	others;

	// flags
	for( int i = 1; i < argc; ){
		std::string arg = argv[i++];

		if( arg.find( "--" ) == 0 ){
			arg = arg.erase( 0, 2 );
			
			if( auto option = d->find( arg ) ){
				registerUse( arg );
				if( !option->mParamName.empty() ){
					// discard data: config file and defaults
					option->mValues.clear();
					while( i < argc ){
						std::string subArg = argv[i++];
						if( subArg.find( "-" ) == 0 ){
							break;
						}else{
							set( option->mName, subArg );
						}
					}
					if( option->mValues.empty() ){
						spdlog::warn( "Missing arguments for flag {}", arg );
					}
				}
			}else{
				reportUnknownArgument( arg );
			}
		}else{
			others.push_back( arg );
		}
	}
	return others;
}

bool drea::core::Config::used( const std::string & optionName ) const
{
	return std::find( d->mFlags.begin(), d->mFlags.end(), optionName ) != d->mFlags.end();
}

void drea::core::Config::registerUse( const std::string & optionName )
{
	if( !used( optionName ) ){
		d->mFlags.push_back( optionName );
	}
}

void drea::core::Config::set( const std::string & optionName, const std::string & value )
{
	d->set( optionName, value );
}

void drea::core::Config::append( const std::string & optionName, const std::string & value )
{
	d->append( optionName, value );
}

std::shared_ptr<spdlog::logger> drea::core::Config::setupLogger() const
{
	std::shared_ptr<spdlog::logger>		res;
	std::vector<spdlog::sink_ptr> 		sinks;
	std::string							logFile = get<std::string>( "log-file" );

	sinks.push_back( std::make_shared<spdlog::sinks::stdout_color_sink_st>() );
	if( !logFile.empty() ){
		sinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>( logFile, 1048576 * 5, 3 ) );
	}
	if( used( "graylog-host" ) ){
		sinks.push_back( std::make_shared< drea::core::integrations::logs::graylog_sink<spdlog::details::null_mutex>>( App::instance().name(), get<std::string>( "graylog-host" ) ) );
	}
	res = std::make_shared<spdlog::logger>( App::instance().name(), sinks.begin(), sinks.end() );
	if( used( "verbose" ) ){
		res->set_level( spdlog::level::debug );
	}
	return res;
}

void drea::core::Config::reportUnknownArgument( const std::string & optionName ) const
{
	size_t			bestDist = 0;
	std::string		bestArg;

	for( const auto & opt: d->mOptions ){
		size_t	nd = levenshtein( optionName, opt->mName );
		if( bestArg.empty() || nd < bestDist ){
			bestDist = nd;
			bestArg = opt->mName;
		}
	}
	spdlog::warn( "Unknown argument \"{}\". Did you mean \"{}\"?", optionName, bestArg );
}
