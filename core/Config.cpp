
#include <vector>
#include <optional>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <fstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/fmt/fmt.h>
#include <woorm/levenshtein.h>

#include "integrations/yaml/yaml_reader.h"
#include "integrations/json/json_reader.h"
#include "integrations/toml/toml_reader.h"

#ifdef ENABLE_REST_USE
	#include "integrations/graylog/graylog_sink.h"
	#include "integrations/etcd/kv_store.h"
	#include <consulcpp/ConsulCpp>
#endif

#include "Config.h"
#include "App.h"

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
	if( env_p != nullptr ){
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
	std::string								mDefaultConfigFile;
	std::vector<std::string>				mFlags;
	std::vector<std::unique_ptr<Option>>	mOptions;
	std::string								mEnvPrefix;
	std::vector<RemoteProvider>				mRemoteProviders;
	App										& mApp;

	Private( App & app ) : mApp( app )
	{
	}

	jss::object_ptr<Option> find( const std::string & optionName ){
		jss::object_ptr<Option>	res;

		if( !optionName.empty() ){
			for( const auto & opt: mOptions ){
				if( opt->mName == optionName || opt->mShortVersion == optionName ){
					res = opt;
					break;
				}
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
		bool		res = false;

		if( !val.empty() ){
			if( !res && drea::core::integration::toml::Reader().valid( val ) ){
				drea::core::integration::toml::Reader().readConfig( mApp, val );
				res = true;
			}
			if( !res && drea::core::integration::json::Reader().valid( val ) ){
				drea::core::integration::json::Reader().readConfig( mApp, val );
				res = true;
			}
			if( !res && drea::core::integration::yaml::Reader().valid( val ) ){
				drea::core::integration::yaml::Reader().readConfig( mApp, val );
				res = true;
			}
		}else{
			res = true;
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

	void readConfig( const std::vector<std::string> & args )
	{
		std::string configFileName = mDefaultConfigFile;

		for( int i = 0; i < int(args.size())-1; i++ ){
			if( std::string( args.at( i ) ) == "--config-file" ){
				configFileName = args.at( i+1 );
				break;
			}
		}
		if( !configFileName.empty() ){
			auto fileData = readFile( configFileName );
			if( !fileData.empty() ){
				if( !readConfig( fileData ) ){
					spdlog::error( "Cannot determine the format of the config file {}", configFileName );
				}
			}
		}
	}
};

drea::core::Config::Config( drea::core::App & app ) : d( std::make_unique<Private>( app ) )
{
}

drea::core::Config::~Config()
{
}

void drea::core::Config::setDefaultConfigFile( const std::string & filePath )
{
	add({
		"config-file", "file", "read configs from file <file>", {}, typeid( std::string )
	});
	if( !filePath.empty() ){
		d->mDefaultConfigFile = filePath;
		set( "config-file", filePath );
	}
}

drea::core::Config & drea::core::Config::addDefaults()
{
	add({
		{
			"verbose", "", "increase the logging level to debug"
		},
		{
			"help", "", "show help and quit"
		},
		{
			"version", "", "print version information and quit"
		},
		{
			"log-file", "file", "log messages to the file <file>", {}, typeid( std::string )
		},
		{
			"log-folder", "folder", "log messages to a file in <folder>", {}, typeid( std::string )
		},
#ifdef ENABLE_REST_USE
		{
			"graylog-host", "schema://host:port", "Send logs to a graylog server. Example: http://localhost:12201", {}, typeid( std::string )
		}
#endif
	});
	if( d->mDefaultConfigFile.empty() ){
		setDefaultConfigFile( {} );
	}
	find( "verbose" )->mShortVersion = "v";
	find( "verbose" )->mNbParams = 0;
	find( "help" )->mShortVersion = "h";
	find( "help" )->mNbParams = 0;
	find( "version" )->mShortVersion = "V";
	find( "version" )->mNbParams = 0;

	return *this;
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

void drea::core::Config::add( const std::vector<drea::core::Option> & options )
{
	for( const auto & option: options ){
		add( option );
	}
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

void drea::core::Config::configure( const std::vector<std::string> & args )
{
	// Order (lower to higher)
	// - defaults
	// - KV Store (as Consul or etcd)
	// - config file
	// - env variables
	// - command line flags
	// - explicit call to set

	// order options alphabetically
	std::sort( d->mOptions.begin(), d->mOptions.end(), []( const auto & a, const auto & b ){ return a->mName < b->mName; });

	// Add values with defaults
	for( const auto & option: d->mOptions ){
		if( !option->mValues.empty() ){
			registerUse( option->mName );
		}
	}
	// KV Store
#ifdef ENABLE_REST_USE
	for( const RemoteProvider & provider: d->mRemoteProviders ){
		if( provider.mProvider == "consul" ){
			consulcpp::Consul	consul( provider.mHost );
			auto				valueMaybe = consul.kv().get( provider.mKey );

			if( valueMaybe ){
				d->readConfig( valueMaybe.value() );
			}
		}else if( provider.mProvider == "etcd" ){
			d->readConfig( integrations::etcd::KVStore( provider.mHost ).get( provider.mKey ) );
		}
	}
#endif
	// Read the config file
	d->readConfig( args );

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

	// flags
	for( int i = 0; i < args.size(); ){
		std::string arg = args.at( i++ );

		if( arg.find( "--" ) == 0 ){
			arg = arg.erase( 0, 2 );
			
			if( auto option = d->find( arg ) ){
				registerUse( arg );
				option->mValues.clear();
				for( int np = 0; np < option->numberOfParams() && i < args.size(); np++ ){
					std::string subArg = args.at( i );
					if( subArg.find( "-" ) == 0 ){
						break;
					}
					append( option->mName, subArg );
					i++;
				}
				if( option->numberOfParams() > 0 &&  option->mValues.empty() ){
					spdlog::warn( "Missing arguments for flag {}", arg );
				}
			}else{
				reportUnknownArgument( arg );
			}
		}
	}
}

bool drea::core::Config::used( const std::string & optionName ) const
{
	return std::find( d->mFlags.begin(), d->mFlags.end(), optionName ) != d->mFlags.end();
}

unsigned int drea::core::Config::intensity( const std::string & optionName ) const
{
	return std::count( d->mFlags.begin(), d->mFlags.end(), optionName );
}

void drea::core::Config::registerUse( const std::string & optionName )
{
	auto option = find( optionName );
	if( option ){
		// can repeat only options without arguments to increase intensity
		bool	canBeIntense = option->mNbParams == 0;

		if( canBeIntense || !used( optionName ) ){
			d->mFlags.push_back( optionName );
		}
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

	sinks.push_back( std::make_shared<spdlog::sinks::stdout_color_sink_mt>() );
	if( logFile.empty() ){
		std::string						logFolder = get<std::string>( "log-folder" );
		
		if( !logFolder.empty() ){
			logFile = fmt::format( "{}/{}.log", logFolder, d->mApp.name() );
		}
	}
	if( !logFile.empty() ){
		sinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>( logFile, 1048576 * 5, 3 ) );
	}
#ifdef ENABLE_REST_USE	
	if( used( "graylog-host" ) ){
		sinks.push_back( std::make_shared< drea::core::integrations::logs::graylog_sink<spdlog::details::null_mutex>>( d->mApp.name(), get<std::string>( "graylog-host" ) ) );
	}
#endif
	res = std::make_shared<spdlog::logger>( d->mApp.name(), sinks.begin(), sinks.end() );
	if( intensity( "verbose" ) == 1 ){
		res->set_level( spdlog::level::debug );
	}else if( intensity( "verbose" ) > 1 ){
		res->set_level( spdlog::level::trace );
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
