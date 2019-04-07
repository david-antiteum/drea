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
	d->mLogger  = d->mConfig.setupLogger();

	d->mCommander.configure( others );
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
