#include "Commander.h"
#include "App.h"
#include "Config.h"

#include <vector>
#include <algorithm>
#include <spdlog/fmt/fmt.h>

struct drea::core::Commander::Private
{
	std::string						mCommand;
	std::vector<std::string>		mArguments;
	std::vector<Command>			mCommands;
};

drea::core::Commander::Commander()
{
	d = std::make_unique<Private>();
}

drea::core::Commander::~Commander()
{
}

drea::core::Commander & drea::core::Commander::addDefaults()
{
	return add(
		{
			"version", "show app version and quits"
		} 
	).add(
		{
			"help", "show app help and quits"
		}
	);
}

drea::core::Commander & drea::core::Commander::add( drea::core::Command cmd )
{
	d->mCommands.push_back( cmd );

	return *this;
}

void drea::core::Commander::configure( const std::vector<std::string> & args )
{
	if( !args.empty() ){
		d->mCommand = args[0];
		for( int i = 1; i < args.size(); i++ ){
			if( args[i][0] != '-' ){
				d->mArguments.push_back( args[i] );
			}
		}
	}
}

void drea::core::Commander::run( std::function<void( std::string )> f )
{
	if( d->mCommand == "version" || App::instance().config().contains( "version" )){
		App::instance().showVersion();
	}else if( d->mCommand == "help" || App::instance().config().contains( "help" ) ){
		App::instance().showHelp();
	}else if( !d->mCommand.empty() ){
		f( d->mCommand );
	}
}

std::vector<std::string> drea::core::Commander::arguments()
{
	return d->mArguments;
}

void drea::core::Commander::showHelp()
{
	if( d->mCommands.empty() ){
		fmt::print( "This app has no commands\n" );
	}else{
		fmt::print( "Commands:\n\n" );

		int offset = 1;
		for( const Command & command: d->mCommands ){
			offset = std::max<int>( offset, command.name.size() + 2 );
		}
		for( const Command & command: d->mCommands ){
			fmt::print( "- {:<{}}", command.name, offset );
			fmt::print( "{}\n", command.description );
		}
	}
}
