#include "Config.h"

#include <vector>
#include <optional>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <iostream>

struct drea::core::Config::Private
{
	std::vector<std::string>		mFlags;
	std::vector<Option>				mOptions;

	std::optional<Option*> find( const std::string & flag ){
		std::optional<Option*>	res;

		for( Option & opt: mOptions ){
			if( opt.name == flag ){
				res = &opt;
				break;
			}
		}
		return res;
	}
};

drea::core::Config::Config()
{
	d = std::make_unique<Private>();
}

drea::core::Config::~Config()
{
}

drea::core::Config & drea::core::Config::addDefaults()
{
	return add(
		{
			"verbose"
		}
	).add(
		{
			"help"
		}
	).add(
		{
			"version"
		}
	).add(
		{
			"log-file", "file name", "log messages to the file <file name>" 
		}
	);
}

drea::core::Config & drea::core::Config::add( drea::core::Option option )
{
	d->mOptions.push_back( option );
	return *this;
}

std::vector<std::string> drea::core::Config::configure( int argc, char * argv[] )
{
	std::vector<std::string>	others;

	for( int i = 1; i < argc; ){
		std::string  arg = argv[i++];

		if( arg.find( "--" ) == 0 ){
			arg = arg.erase( 0, 2 );
			
			if( auto option = d->find( arg ) ){
				d->mFlags.push_back( arg );
				if( !option.value()->paramName.empty() ){
					while( i < argc ){
						std::string subArg = argv[i];
						
						if( subArg.find( "-" ) == 0 ){
							break;
						}else{
							option.value()->values.push_back( subArg );
							i++;
						}
					}
					if( option.value()->values.empty() ){
						spdlog::warn( "Missing arguments for flag {}", arg );
					}
				}
			}else{
				spdlog::warn( "Unknown argument {}", arg );
			}
		}else{
			others.push_back( arg );
		}
	}
	// Add values with defaults not in argv
	for( const Option & option: d->mOptions ){
		if( !option.values.empty() && !contains( option.name )){
			d->mFlags.push_back( option.name );
		}
	}
	return others;
}

bool drea::core::Config::contains( const std::string & flag ) const
{
	return std::find( d->mFlags.begin(), d->mFlags.end(), flag ) != d->mFlags.end();
}

std::string drea::core::Config::value( const std::string & flag ) const
{
	std::string				res;
	std::optional<Option*> 	option = d->find( flag );
			
	if( option ){
		if( !option.value()->values.empty() ){
			res = option.value()->values.front();
		}
	}
	return res;
}

void drea::core::Config::showHelp( int offset )
{
	for( const Option & option: d->mOptions ){
		for( int i = 0; i < offset; i++ ){
			std::cout << " ";
		}
		if( option.paramName.empty() ){
			fmt::print( "[--{}] {}\n", option.name, option.description );
		}else{
			fmt::print( "[--{} ", option.name );
			fmt::print( "<{}>", option.paramName );
			fmt::print( "] {}\n", option.description );
		}
	}
	fmt::print( "\n" );
}
