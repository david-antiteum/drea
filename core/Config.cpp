
#include <vector>
#include <optional>
#include <iostream>
#include <map>
#include <memory>
#include <stdlib.h>
#include <cctype>
#include <fstream>

#if defined(__cpp_lib_filesystem)
#   define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 0
#elif defined(__cpp_lib_experimental_filesystem)
#   define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#elif !defined(__has_include)
#   define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#elif __has_include(<filesystem>)
#   define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 0
#elif __has_include(<experimental/filesystem>)
#   define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#else
#   error Could not find system header "<filesystem>" or "<experimental/filesystem>"
#endif

#if INCLUDE_STD_FILESYSTEM_EXPERIMENTAL
#   include <experimental/filesystem>
namespace std {
    namespace filesystem = experimental::filesystem;
}
#else
#   include <filesystem>
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/fmt/fmt.h>
#include <woorm/levenshtein.h>
#include <boost/algorithm/string.hpp>

#include "integrations/yaml/yaml_reader.h"
#include "integrations/json/json_reader.h"
#include "integrations/toml/toml_reader.h"
#include "integrations/logs/json_formatter.h"
#include "integrations/logs/text_fields_formatter.h"
#include <drea/log/Logger.h>
#include <drea/log/Redacted.h>

#ifdef ENABLE_REST_USE
	#include "integrations/graylog/graylog_sink.h"
#endif

#ifdef ENABLE_AWS
	#include "integrations/aws/secrets_manager.h"
#endif

#include "Config.h"
#include "App.h"
#include "Commander.h"
#include <drea/core/Command.h>

namespace drea::core {

static std::string getenvByName( const std::string & varName )
{
	std::string		res;
	char			*env_p = nullptr;
#ifdef WIN32
	size_t 	sz = 0;
	if( _dupenv_s( &env_p, &sz, varName.c_str() ) == 0 && env_p ){
		res = env_p;
	}
	free( env_p );
#else
	env_p = ::getenv( varName.c_str() );
	if( env_p != nullptr ){
		res = env_p;
	}
#endif
	return res;
}

static std::string getenv( const std::string & prefix, const std::string & name )
{
	std::string		res = getenvByName( prefix + "_" + name );

	if( res.empty() ){
		// option names may contain characters that are invalid in shell variable
		// names (e.g. "config-file", "db.host"): map them to '_'
		std::string		sanitized = name;

		for( char & c: sanitized ){
			if( !std::isalnum( static_cast<unsigned char>( c ) ) && c != '_' ){
				c = '_';
			}
		}
		if( sanitized != name ){
			res = getenvByName( prefix + "_" + sanitized );
		}
		if( res.empty() ){
			// also accept the conventional all-uppercase spelling
			std::string		upper = sanitized;

			for( char & c: upper ){
				c = static_cast<char>( std::toupper( static_cast<unsigned char>( c ) ) );
			}
			if( upper != sanitized ){
				res = getenvByName( prefix + "_" + upper );
			}
		}
	}
	return res;
}

}

struct drea::core::Config::Private
{
	std::string								mDefaultConfigFile;
	std::vector<std::string>				mFlags;
	std::vector<std::unique_ptr<Option>>	mOptions;
	std::string								mEnvPrefix;
	// which stage of Config::configure provided the current value, per option
	std::map<std::string, std::string>		mSources;
	std::string								mCurrentSource = "default";
	// declared defaults, snapshot by Config::configure before sources overwrite them
	std::map<std::string, std::vector<OptionValue>>	mDeclaredDefaults;
	// problems collected while sources are applied; Config::findings adds the declarative checks
	std::vector<Config::Finding>			mFindings;
	// the config file being read, for unknown-key messages
	std::string								mActiveConfigFile;
	App										& mApp;

	explicit Private( App & app ) : mApp( app )
	{
	}

	jss::object_ptr<Option> find( std::string_view optionName ){
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
				mSources[ option->mName ] = mCurrentSource;
			}else if( mCurrentSource == "code" ){
				// Config::set after parsing keeps its documented contract:
				// fromString already reported, exit
				exit( -1 );
			}else{
				// collect and continue: validation reports every problem
				// after source resolution instead of dying on the first one
				mFindings.push_back( { option->mName, mCurrentSource, "parse_error",
					fmt::format( "Option --{} value {} is not a valid {}", option->mName,
						option->mSensitive ? "[redacted]" : value, option->typeName() ) } );
			}
		}
	}

	bool readConfigTOML( const std::string & val )
	{
		bool		res = false;

		if( !val.empty() ){
			if( drea::core::integration::toml::Reader().valid( val ) ){
				drea::core::integration::toml::Reader().readConfig( mApp, val );
				res = true;
			}
		}else{
			res = true;
		}
		return res;
	}

	bool readConfigJSON( const std::string & val )
	{
		bool		res = false;

		if( !val.empty() ){
			if( drea::core::integration::json::Reader().valid( val ) ){
				drea::core::integration::json::Reader().readConfig( mApp, val );
				res = true;
			}
		}else{
			res = true;
		}
		return res;
	}

	bool readConfigYAML( const std::string & val )
	{
		bool		res = false;

		if( !val.empty() ){
			if( drea::core::integration::yaml::Reader().valid( val ) ){
				drea::core::integration::yaml::Reader().readConfig( mApp, val );
				res = true;
			}
		}else{
			res = true;
		}
		return res;
	}

	bool readConfig( const std::string & val )
	{
		bool		res = false;

		res = res || readConfigTOML( val );
		res = res || readConfigJSON( val );
		res = res || readConfigYAML( val );

		return res;
	}

	std::pair<std::string, std::string> readFile( const std::string & configFileName )
	{
		std::string		res, extension;
		std::ifstream 	configFile;
		auto			path = std::filesystem::u8path( configFileName );

		configFile.open( path, std::ios::in );
		if( configFile.is_open() ){
			std::stringstream buffer;
			buffer << configFile.rdbuf();
			res = buffer.str();
			if( res.empty() ){
				spdlog::warn( "The config file {} is empty", configFileName );
			}
		}else{
			spdlog::error( "Cannot read the config file {}", configFileName );
			mFindings.push_back( { "config-file", mCurrentSource, "file_error",
				fmt::format( "Cannot read the config file {}", configFileName ) } );
		}
		return { res, boost::algorithm::to_lower_copy( path.extension().string() ) };
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
			mActiveConfigFile = configFileName;
			auto [fileData, extension] = readFile( configFileName );
			if( !fileData.empty() ){
				bool	ok = true;

				if( extension == ".toml" ){
					ok = readConfigTOML( fileData );
				}else if( extension == ".json" ){
					ok = readConfigJSON( fileData );
				}else if( extension == ".yaml" || extension == ".yml" ){
					ok = readConfigYAML( fileData );
				}else if( !readConfig( fileData ) ){
					spdlog::error( "Cannot determine the format of the config file {}", configFileName );
					ok = false;
				}
				if( !ok ){
					mFindings.push_back( { "config-file", mCurrentSource, "parse_error",
						fmt::format( "The config file {} cannot be parsed", configFileName ) } );
				}
			}
		}
	}

	// Parses "aws://<region>/<secret-id>". An empty region means "use the SDK default chain".
	// A value with no '/' after the scheme is treated as the secret id with no explicit region.
	static std::pair<std::string, std::string> parseAwsUri( const std::string & uri )
	{
		constexpr std::string_view prefix = "aws://";
		std::string rest = uri.substr( prefix.size() );
		if( rest.empty() ){
			return {};
		}
		auto slash = rest.find( '/' );
		if( slash == std::string::npos ){
			return { std::string(), rest };
		}
		return { rest.substr( 0, slash ), rest.substr( slash + 1 ) };
	}

	void fetchConfigSource( const std::string & uri )
	{
		constexpr std::string_view awsPrefix = "aws://";
		if( uri.size() >= awsPrefix.size() && uri.compare( 0, awsPrefix.size(), awsPrefix ) == 0 ){
#ifdef ENABLE_AWS
			auto [region, secretId] = parseAwsUri( uri );
			if( secretId.empty() ){
				spdlog::error( "Invalid config-source URI '{}': missing secret id", uri );
				return;
			}
			readConfigJSON( integrations::aws::SecretsManager( region ).get( secretId ) );
#else
			spdlog::error( "config-source '{}' requires drea built with ENABLE_AWS", uri );
#endif
		}else{
			spdlog::warn( "Unsupported config-source scheme: '{}'", uri );
		}
	}

	void readConfigSources( const std::vector<std::string> & args )
	{
		for( int i = 0; i < int(args.size()) - 1; i++ ){
			if( args.at( i ) == "--config-source" ){
				fetchConfigSource( args.at( i + 1 ) );
			}
		}
	}
};

drea::core::Config::Config( drea::core::App & app ) : d( std::make_unique<Private>( app ) )
{
}

drea::core::Config::~Config() = default;

void drea::core::Config::setDefaultConfigFile( const std::string & filePath )
{
	add({
		"config-file", "file", "read configs from file <file>", {}, typeid( std::string )
	});
	find( "config-file" )->mPredefined = true;
	if( !filePath.empty() ){
		d->mDefaultConfigFile = filePath;
		set( "config-file", filePath );
	}
}

drea::core::Config & drea::core::Config::addDefaults()
{
	std::string logFolderInfo = fmt::format( "log messages to file {}.log in <folder>", d->mApp.name() );

	add({
		{
			"verbose", "", "increase the logging level to debug", {}, typeid( bool )
		},
		{
			"help", "", "show help and quit", {}, typeid( bool )
		},
		{
			"version", "", "print version information and quit", {}, typeid( bool )
		},
		{
			"describe", "", "print the app description (commands, options and limits) as JSON and quit", {}, typeid( bool )
		},
		{
			"validate", "", "check the configuration resolved from all sources, report every problem and quit", {}, typeid( bool )
		},
		{
			"json", "", "machine readable output (JSON) where supported (--validate)", {}, typeid( bool )
		},
		{
			"config-source", "uri", "read configs from a remote source. Can be repeated. "
			                       "Supported schemes: aws://<region>/<secret-id> (requires ENABLE_AWS)",
			{}, typeid( std::string )
		},
		{
			"log-file", "file", "log messages to the file <file>", {}, typeid( std::string )
		},
		{
			"log-folder", "folder", logFolderInfo, {}, typeid( std::string )
		},
		{
			"log-size", "size", "log <size> (in MB) for each log file", {10}, typeid( int )
		},
		{
			"log-nb-files", "number-of-log-files", "<number-of-log-files> to keep", {10}, typeid( int )
		},
		{
			"log-flush-level", "level", "flush log sinks on messages of <level> or above", {std::string("warn")}, typeid( std::string )
		},
		{
			"log-redact", "", "redact values wrapped in drea::log::redacted(); use --no-log-redact to see them", {true}, typeid( bool )
		},
		{
			"log-config", "", "log the effective configuration (value and source per option) after parsing", {false}, typeid( bool )
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
	find( "describe" )->mNbParams = 0;
	find( "validate" )->mNbParams = 0;
	find( "json" )->mNbParams = 0;
	// the set Config::setupLogger accepts; validation rejects anything else
	// before the logger silently falls back to warn
	find( "log-flush-level" )->mChoices = { "trace", "debug", "info", "warn", "err", "critical", "off" };

	for( const char * name: {
		"verbose", "help", "version", "describe", "validate", "json",
		"config-source", "config-file",
		"log-file", "log-folder", "log-size", "log-nb-files",
		"log-flush-level", "log-redact", "log-config"
#ifdef ENABLE_REST_USE
		, "graylog-host"
#endif
	} ){
		if( auto option = find( name ) ){
			option->mPredefined = true;
		}
	}

	return *this;
}

void drea::core::Config::remove( std::string_view optionName )
{
	const std::string	name( optionName );
	d->mOptions.erase(
		std::remove_if( d->mOptions.begin(), d->mOptions.end(),
			[&name]( const std::unique_ptr<Option> & opt ){
				return opt->mName == name;
			} ),
		d->mOptions.end() );
	d->mFlags.erase(
		std::remove( d->mFlags.begin(), d->mFlags.end(), name ),
		d->mFlags.end() );
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

const std::string & drea::core::Config::envPrefix() const
{
	return d->mEnvPrefix;
}

jss::object_ptr<drea::core::Option> drea::core::Config::find( std::string_view optionName ) const
{
	return d->find( optionName );
}

void drea::core::Config::configure( const std::vector<std::string> & args )
{
	// Order (lower to higher)
	// - defaults
	// - remote config sources (--config-source)
	// - config file
	// - env variables
	// - command line flags
	// - explicit call to set

	// order options alphabetically
	std::sort( d->mOptions.begin(), d->mOptions.end(), []( const auto & a, const auto & b ){ return a->mName < b->mName; });

	d->mFindings.clear();
	d->mDeclaredDefaults.clear();
	// Add values with defaults, and snapshot them before sources overwrite
	// them: the resolved-config model keeps value, source and declared default
	for( const auto & option: d->mOptions ){
		if( !option->mValues.empty() ){
			registerUse( option->mName );
			d->mSources[ option->mName ] = "default";
			d->mDeclaredDefaults[ option->mName ] = option->mValues;
		}
	}
	// Remote config sources: --config-source <uri> (repeatable).
	d->mCurrentSource = "config-source";
	d->readConfigSources( args );

	// Read the config file
	d->mCurrentSource = "config-file";
	d->readConfig( args );

	// Env vars. The environment belongs to the config sources: options scoped
	// to the command line only (or to no user source at all) don't read it.
	d->mCurrentSource = "environment";
	if( !d->mEnvPrefix.empty() ){
		for( const auto & option: d->mOptions ){
			if( option->mScope == Option::Scope::Line || option->mScope == Option::Scope::None ){
				continue;
			}
			std::string		env = drea::core::getenv( d->mEnvPrefix, option->mName );
			if( !env.empty() ){
				registerUse( option->mName );
				if( !option->mParamName.empty() ){
					set( option->mName, env );
				}else{
					d->mSources[ option->mName ] = d->mCurrentSource;
				}
			}
		}
	}
	d->mCurrentSource = "flag";

	// What defaults we have?
	std::set<jss::object_ptr<drea::core::Option>> optionsWithDefault;

	for( const auto & option: d->mOptions ){
		if( !option->mValues.empty() ){
			optionsWithDefault.insert( option );
		}
	}
	// flags
	for( size_t i = 0; i < args.size(); ){
		std::string arg = args.at( i++ );

		if( arg.find( "--" ) == 0 ){
			arg = arg.erase( 0, 2 );

			std::string inlineValue;
			bool hasInlineValue = false;
			if( auto eq = arg.find( '=' ); eq != std::string::npos ){
				inlineValue = arg.substr( eq + 1 );
				arg = arg.substr( 0, eq );
				hasInlineValue = true;
			}

			if( auto option = d->find( arg ) ){
				registerUse( arg );
				d->mSources[ option->mName ] = "flag";
				if( optionsWithDefault.count( option ) > 0 ){
					// override default. Clean value and keep values from user
					option->mValues.clear();
					optionsWithDefault.erase( option );
				}
				if( hasInlineValue ){
					if( option->numberOfParams() > 0 ){
						append( option->mName, inlineValue );
					}else{
						spdlog::warn( "Flag {} does not take a value; ignoring '={}'", arg, inlineValue );
					}
				}else{
					for( int np = 0; np < option->numberOfParams() && i < args.size(); np++ ){
						std::string subArg = args.at( i );
						if( subArg.find( "-" ) == 0 ){
							break;
						}
						append( option->mName, subArg );
						i++;
					}
				}
				if( option->numberOfParams() > 0 &&  option->mValues.empty() ){
					spdlog::warn( "Missing arguments for flag {}", arg );
				}else if( option->numberOfParams() == 0 && option->mType == typeid( bool ) ){
					option->mValues.clear();
					option->mValues.push_back( true );
				}
			}else if( arg.rfind( "no-", 0 ) == 0 ){
				std::string boolName = arg.substr( 3 );
				if( auto boolOpt = d->find( boolName ); boolOpt && boolOpt->mType == typeid( bool ) ){
					registerUse( boolName );
					d->mSources[ boolOpt->mName ] = "flag";
					boolOpt->mValues.clear();
					boolOpt->mValues.push_back( false );
					optionsWithDefault.erase( boolOpt );
				}else{
					reportUnknownArgument( arg );
				}
			}else{
				reportUnknownArgument( arg );
			}
		}
	}
	// values set by the app after configure (Config::set) come from code
	d->mCurrentSource = "code";
}

bool drea::core::Config::used( const std::string & optionName ) const
{
	return std::find( d->mFlags.begin(), d->mFlags.end(), optionName ) != d->mFlags.end();
}

unsigned int drea::core::Config::intensity( const std::string & optionName ) const
{
	return static_cast<unsigned int>(std::count( d->mFlags.begin(), d->mFlags.end(), optionName ));
}

void drea::core::Config::registerUse( const std::string & optionName )
{
	if( auto option = find( optionName ); option ){
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
	registerUse( optionName );
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

	// Both formatters read spdlog::mdc at format time, on the calling thread.
	// NEVER switch this function to spdlog::async_logger: formatting would
	// move to a backend thread and every MDC read would silently return an
	// empty map, dropping all structured fields.
	auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto consoleFormatter = std::make_unique<spdlog::pattern_formatter>();

	// spdlog's default pattern plus %*: structured fields as [key:value]
	// blocks, nothing when there are none
	consoleFormatter->add_flag<integrations::logs::text_fields_flag>( '*' ).set_pattern( "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %*%v" );
	consoleSink->set_formatter( std::move( consoleFormatter ) );
	sinks.push_back( consoleSink );
	if( logFile.empty() ){
		std::string						logFolder = get<std::string>( "log-folder" );
		
		if( !logFolder.empty() ){
			logFile = fmt::format( "{}/{}.log", logFolder, d->mApp.name() );
		}
	}
	if( !logFile.empty() ){
		int max_size = 1048576 * get<int>( "log-size" );
		int max_files = get<int>( "log-nb-files" );

		try{
			auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>( logFile, max_size, max_files );

			// structured logs to file, human text to console: the JSON formatter
			// goes on this sink only
			fileSink->set_formatter( std::make_unique<integrations::logs::json_lines_formatter>() );
			sinks.push_back( fileSink );
		}catch( spdlog::spdlog_ex & se ){
			fmt::print( "Cannot use log file {}: {}\n", logFile, se.what() );
		}catch( std::exception & e ){
			fmt::print( "Cannot use log file {}: {}\n", logFile, e.what() );
		}
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

	auto flushLevel = spdlog::level::warn;

	if( std::string flushName = get<std::string>( "log-flush-level" ); !flushName.empty() ){
		if( auto parsed = spdlog::level::from_str( flushName ); parsed != spdlog::level::off && flushName != "off" ){
			flushLevel = parsed;
		}else if( flushName == "off" ){
			flushLevel = spdlog::level::off;
		}else{
			spdlog::warn( "Unknown log-flush-level \"{}\", using \"warn\"", flushName );
		}
	}
	res->flush_on( flushLevel );

	// flush_every only reaches loggers held by the registry
	spdlog::drop( res->name() );
	spdlog::register_logger( res );
	spdlog::flush_every( std::chrono::seconds( 3 ) );

	// read once at startup and frozen: config is immutable per house rules.
	// Without the option (addDefaults not used) redaction stays on.
	if( find( "log-redact" ) ){
		drea::log::detail::setRedactionEnabled( get<bool>( "log-redact" ) );
	}

	return res;
}

std::vector<std::string> drea::core::Config::validate() const
{
	std::vector<std::string>	errors;

	// the fatal subset of the findings: --validate reports them all
	for( const auto & finding: findings() ){
		if( finding.mCode == "parse_error" || finding.mCode == "missing_required" || finding.mCode == "bad_choice"
			|| finding.mCode == "out_of_range" || finding.mCode == "unknown_option_ref" ){
			errors.push_back( finding.mMessage );
		}
	}
	return errors;
}

std::vector<drea::core::Config::Finding> drea::core::Config::findings() const
{
	std::vector<Finding>	res = d->mFindings;

	for( const auto & option: d->mOptions ){
		const std::string	src = source( option->mName );

		if( option->mRequired && option->mValues.empty() ){
			res.push_back( { option->mName, {}, "missing_required", fmt::format( "Missing required option --{}", option->mName ) } );
		}
		if( !option->mChoices.empty() ){
			for( const auto & value: option->mValues ){
				if( const std::string asText = option->toString( value ); std::find( option->mChoices.begin(), option->mChoices.end(), asText ) == option->mChoices.end() ){
					res.push_back( { option->mName, src, "bad_choice", fmt::format( "Option --{} value {} is not one of: {}", option->mName,
						option->mSensitive ? "[redacted]" : asText, boost::algorithm::join( option->mChoices, ", " ) ) } );
				}
			}
		}
		if( option->mMin || option->mMax ){
			for( const auto & value: option->mValues ){
				std::optional<double>	num;

				if( std::holds_alternative<int>( value ) ){
					num = std::get<int>( value );
				}else if( std::holds_alternative<double>( value ) ){
					num = std::get<double>( value );
				}
				if( num ){
					const std::string asText = option->mSensitive ? "[redacted]" : fmt::format( "{}", *num );

					if( option->mMin && *num < *option->mMin ){
						res.push_back( { option->mName, src, "out_of_range", fmt::format( "Option --{} value {} is below the minimum {}", option->mName, asText, *option->mMin ) } );
					}
					if( option->mMax && *num > *option->mMax ){
						res.push_back( { option->mName, src, "out_of_range", fmt::format( "Option --{} value {} is above the maximum {}", option->mName, asText, *option->mMax ) } );
					}
				}
			}
		}
		// checks on the source that provided the value: only when a real
		// source (not the declared default) set the option
		if( const auto it = d->mSources.find( option->mName ); it != d->mSources.end() && it->second != "default" && it->second != "code" ){
			const bool	fromConfigSources = src == "config-source" || src == "config-file" || src == "environment";

			if( src == "flag" && ( option->mScope == Option::Scope::File || option->mScope == Option::Scope::None ) ){
				res.push_back( { option->mName, src, "wrong_scope",
					fmt::format( "Option --{} has scope {} and may not be set from the command line", option->mName, option->scopeName() ) } );
			}else if( fromConfigSources && ( option->mScope == Option::Scope::Line || option->mScope == Option::Scope::None ) ){
				res.push_back( { option->mName, src, "wrong_scope",
					fmt::format( "Option --{} has scope {} and may not be set from config sources or the environment", option->mName, option->scopeName() ) } );
			}
			if( option->numberOfParams() > 0 && option->mNbParams != Option::mUnlimitedParams ){
				if( option->mValues.empty() ){
					res.push_back( { option->mName, src, "missing_params", fmt::format( "Missing arguments for option --{}", option->mName ) } );
				}else if( option->mValues.size() % static_cast<size_t>( option->numberOfParams() ) != 0 ){
					res.push_back( { option->mName, src, "missing_params",
						fmt::format( "Option --{} takes {} values per use, {} given", option->mName, option->numberOfParams(), option->mValues.size() ) } );
				}
			}
		}
	}
	// every option a command references must exist
	d->mApp.commander().commands( [ this, &res ]( const Command & command ){
		for( const auto * list: { &command.mLocalParameters, &command.mGlobalParameters } ){
			for( const auto & optionName: *list ){
				if( !find( optionName ) ){
					res.push_back( { optionName, {}, "unknown_option_ref", fmt::format( "Command \"{}\" references unknown option \"{}\"", command.mName, optionName ) } );
				}
			}
		}
	});
	// the requested command exists but its groups are disabled
	if( const std::string & requested = d->mApp.commander().requestedCommand(); !requested.empty() ){
		if( auto cmd = d->mApp.commander().find( requested ); cmd && !cmd->mHidden && !d->mApp.commander().isVisible( *cmd ) ){
			res.push_back( { requested, {}, "disabled_group",
				fmt::format( "The command \"{}\" is gated by groups that are not enabled ({})", requested, boost::algorithm::join( cmd->mGroups, ", " ) ) } );
		}
	}
	return res;
}

std::vector<drea::core::OptionValue> drea::core::Config::declaredDefault( std::string_view optionName ) const
{
	if( const auto it = d->mDeclaredDefaults.find( std::string( optionName ) ); it != d->mDeclaredDefaults.end() ){
		return it->second;
	}
	return {};
}

std::string drea::core::Config::source( std::string_view optionName ) const
{
	if( auto it = d->mSources.find( std::string( optionName ) ); it != d->mSources.end() ){
		return it->second;
	}
	return "default";
}

void drea::core::Config::logEffective( drea::log::Logger & logger ) const
{
	for( const auto & option: d->mOptions ){
		if( option->mValues.empty() && !used( option->mName ) ){
			continue;
		}
		std::string		value;

		if( option->mSensitive && drea::log::redactionEnabled() ){
			value = "[redacted]";
		}else{
			for( const auto & optionValue: option->mValues ){
				if( !value.empty() ){
					value += ", ";
				}
				value += option->toString( optionValue );
			}
		}
		logger.info( "config: {}={} (from {})", option->mName, value, source( option->mName ) );
	}
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

	std::string		message;

	if( d->mCurrentSource == "config-file" || d->mCurrentSource == "config-source" ){
		if( d->mActiveConfigFile.empty() ){
			message = fmt::format( "Unknown config key \"{}\"", optionName );
		}else{
			message = fmt::format( "Unknown config key \"{}\" in {}", optionName, d->mActiveConfigFile );
		}
	}else{
		message = fmt::format( "Unknown argument \"{}\"", optionName );
	}
	if( !bestArg.empty() ){
		message += fmt::format( ". Did you mean \"{}\"?", bestArg );
	}
	d->mFindings.push_back( { optionName, d->mCurrentSource, "unknown_key", message } );
	spdlog::warn( "{}", message );
}
