#include "Config.h"

#include <vector>
#include <optional>
#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <woorm/levenshtein.h>

#ifdef CXX17FS
#include <filesystem>
#endif
#include <yaml-cpp/yaml.h>
#include <stdlib.h>

#ifdef CPPRESTSDK_ENABLED
	#include "integrations/consul/kv_store.h"
#endif

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
	std::vector<std::string>		mFlags;
	std::vector<Option>				mOptions;
	std::string						mEnvPrefix;
	std::vector<RemoteProvider>		mRemoteProviders;

	jss::object_ptr<Option> find( const std::string & flag ){
		jss::object_ptr<Option>	res;

		for( Option & opt: mOptions ){
			if( opt.mName == flag ){
				res = &opt;
				break;
			}
		}
		return res;
	}

	void set( const std::string & flag, const std::string & value )
	{
		if( auto option = find( flag ) ){
			option->mValues.clear();
			append( flag, value );
		}
	}

	void append( const std::string & flag, const std::string & value )
	{
		if( auto option = find( flag ) ){
			OptionValue	val = option->fromString( value );

			if( val.index() > 0 ){
				option->mValues.push_back( val );
			}else{
				exit( -1 );
			}
		}
	}

	void readConfig( const YAML::Node & config )
	{
		for( auto node: config ){
			std::string arg = node.first.as<std::string>();
			if( auto option = find( arg ) ){
				mFlags.push_back( arg );
				if( !option->mParamName.empty() ){
					// discard data: config file and defaults
					if( node.second.Type() == YAML::NodeType::Scalar ){
						set( option->mName, node.second.as<std::string>() );
					}else if( node.second.Type() == YAML::NodeType::Sequence ){
						option->mValues.clear();
						for( auto seqVal: node.second ){
							if( seqVal.Type() == YAML::NodeType::Scalar ){
								append( option->mName, seqVal.as<std::string>() );
							}
						}
					}
					if( option->mValues.empty() ){
						spdlog::warn( "Missing arguments for flag {}", arg );
					}
				}
			}else{
				reportUnknownArgument( arg );
			}
		}
	}

	void readConfig( const std::string & val )
	{
		if( !val.empty() ){
			readConfig( YAML::Load( val ) );
		}
	}

	void readConfig( int argc, char * argv[] )
	{
#ifdef CXX17FS
		std::filesystem::path		configFile;
#else
		std::string					configFile;
#endif
		for( int i = 1; i < argc-1; i++ ){
			if( std::string( argv[i] ) == "--config-file" ){
				configFile = argv[i+1];
				break;
			}
		}
#ifdef CXX17FS
		if( !configFile.empty() && std::filesystem::exists( configFile ) ){
			YAML::Node config = YAML::LoadFile( configFile.string() );
#else
		if( !configFile.empty() ){
			YAML::Node config = YAML::LoadFile( configFile );
#endif
			readConfig( config );
		}
	}

	void reportUnknownArgument( const std::string & arg ) const
	{
		size_t			bestDist = 0;
		std::string		bestArg;
	
		for( const Option & opt: mOptions ){
			size_t	nd = levenshtein( arg, opt.mName );
			if( bestArg.empty() || nd < bestDist ){
				bestDist = nd;
				bestArg = opt.mName;
			}
		}
		spdlog::warn( "Unknown argument \"{}\". Did you mean \"{}\"?", arg, bestArg );
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
#ifdef CPPRESTSDK_ENABLED
	add(
		{
			"graylog-host", "schema://host:port", "Send logs to a graylog server. Example: http://localhost:12201", {}, typeid( std::string )
		}
	);
#endif
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
	for( const Option & opt: d->mOptions ){
		f( opt );
	}
}

void drea::core::Config::add( drea::core::Option option )
{
	d->mOptions.push_back( option );
}

void drea::core::Config::setEnvPrefix( const std::string & value )
{
	d->mEnvPrefix = value;
}

void drea::core::Config::addRemoteProvider( const std::string & provider, const std::string & host, const std::string & key )
{
	d->mRemoteProviders.push_back( { provider, host, key } );
}

jss::object_ptr<drea::core::Option> drea::core::Config::find( const std::string & flag ) const
{
	return d->find( flag );
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
	for( const Option & option: d->mOptions ){
		if( !option.mValues.empty() ){
			d->mFlags.push_back( option.mName );
		}
	}

	// Read the config file
	d->readConfig( argc, argv );

	// Env vars
	if( !d->mEnvPrefix.empty() ){
		for( Option & option: d->mOptions ){
			std::string		env = drea::core::getenv( d->mEnvPrefix, option.mName );
			if( !env.empty() ){
				d->mFlags.push_back( option.mName );
				if( !option.mParamName.empty() ){
					set( option.mName, env );
				}
			}
		}
	}

	for( const RemoteProvider & provider: d->mRemoteProviders ){
		if( provider.mProvider == "consul" ){
			d->readConfig( integrations::Consul::KVStore( provider.mHost ).get( provider.mKey ) );
		}
	}

	std::vector<std::string>	others;

	// flags
	for( int i = 1; i < argc; ){
		std::string arg = argv[i++];

		if( arg.find( "--" ) == 0 ){
			arg = arg.erase( 0, 2 );
			
			if( auto option = d->find( arg ) ){
				d->mFlags.push_back( arg );
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
				d->reportUnknownArgument( arg );
			}
		}else{
			others.push_back( arg );
		}
	}
	return others;
}

bool drea::core::Config::contains( const std::string & flag ) const
{
	return std::find( d->mFlags.begin(), d->mFlags.end(), flag ) != d->mFlags.end();
}

void drea::core::Config::set( const std::string & flag, const std::string & value )
{
	d->set( flag, value );
}
