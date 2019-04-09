#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include "App.h"
#include "Config.h"
#include "Commander.h"

struct drea::core::App::Private
{
	Private()
	{
	}

	std::string							mAppExeName;
	std::string							mDescription;
	std::string							mVersion = "0.0.0";
	Config								mConfig;
	Commander							mCommander;
	std::shared_ptr<spdlog::logger>		mLogger;
	std::vector<std::string>			mArgs;
};

static drea::core::App * mInstanceApp = nullptr;

drea::core::App::App( int argc, char * argv[] )
{
	d = std::make_unique<Private>();
	mInstanceApp = this;

	if( argc > 0 ){
		for( int i = 0; i < argc; i++ ){
			d->mArgs.push_back( argv[i] );
		}
		d->mAppExeName = argv[0];
	}
}

drea::core::App::~App()
{
	mInstanceApp = nullptr;
}

void drea::core::App::parse()
{
	auto others = d->mConfig.configure( d->mArgs );
	d->mLogger  = d->mConfig.setupLogger();
	d->mCommander.configure( others );
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

spdlog::logger & drea::core::App::logger() const
{
	if( d->mLogger ){
		return *d->mLogger;
	}else{
		return *spdlog::default_logger();
	}
}

std::vector<std::string> drea::core::App::args() const
{
	return d->mArgs;
}
