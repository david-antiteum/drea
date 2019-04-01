#include "App.h"
#include "Config.h"
#include "Commander.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/fmt/fmt.h>

#include <iostream>

struct drea::core::App::Private
{
	Private()
	{
	}

	void setupLogger( const std::string & logFile )
	{
		std::vector<spdlog::sink_ptr> 		sinks;

		sinks.push_back( std::make_shared<spdlog::sinks::stdout_color_sink_st>() );
		if( !logFile.empty() ){
			sinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>( logFile, 1048576 * 5, 3 ) );
		}
//		if( !graylogHost.empty() ){
//			sinks.push_back( std::make_shared<graylog_sink<spdlog::details::null_mutex>>( appName, graylogHost ) );
//		}
		mLogger = std::make_shared<spdlog::logger>( mAppExeName, sinks.begin(), sinks.end() );
	}

	std::string							mAppExeName;
	std::string							mDescription;
	std::string							mVersion = "0.0.0";
	Config								mConfig;
	Commander							mCommander;
	std::shared_ptr<spdlog::logger>		mLogger;
};

static drea::core::App * mInstanceApp = nullptr;

drea::core::App::App()
{
	d = std::make_unique<Private>();
	mInstanceApp = this;
}

void drea::core::App::parse( int argc, char * argv[] )
{
	if( d->mAppExeName.empty() ){
		d->mAppExeName = argv[0];
	}
	auto others = d->mConfig.configure( argc, argv );
	d->mCommander.configure( others );

	d->setupLogger( d->mConfig.get<std::string>( "log-file" ) );
	if( d->mConfig.contains( "verbose" ) ){
		d->mLogger->set_level( spdlog::level::debug );
	}
}

drea::core::App::~App()
{
	mInstanceApp = nullptr;
}

drea::core::App & drea::core::App::instance()
{
	return *mInstanceApp;
}

const std::string & drea::core::App::name() const
{
	return d->mAppExeName;
}

const std::string & drea::core::App::description() const
{
	return d->mDescription;
}

const std::string & drea::core::App::version() const
{
	return d->mVersion;
}

void drea::core::App::setName( const std::string & value )
{
	d->mAppExeName = value;
}

void drea::core::App::setDescription( const std::string & value )
{
	d->mDescription = value;
}

void drea::core::App::setVersion( const std::string & value )
{
	d->mVersion = value;
}

drea::core::Config & drea::core::App::config() const
{
	return d->mConfig;
}

drea::core::Commander & drea::core::App::commander() const
{
	return d->mCommander;
}

std::shared_ptr<spdlog::logger> drea::core::App::logger() const
{
	return d->mLogger;
}

void drea::core::App::showVersion()
{
	std::cout << d->mAppExeName << " version " << d->mVersion << "\n";
	std::exit( 0 );
}

void drea::core::App::showHelp()
{
	std::string::size_type pos = std::string( "usage: " + d->mAppExeName + " " ).size();

	fmt::print( "{}\n\n", description() );

	std::cout << "usage: " << d->mAppExeName << " <command> [<args>]\n";
	d->mConfig.showHelp( static_cast<int>( pos ) );
	d->mCommander.showHelp( {} );

	fmt::print( "\nUse \"{} [command] --help\" for more information about a command.\n", name() );

	std::exit( 0 );
}
