#include "Config.h"

#include <vector>
#include <optional>
#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <filesystem>
#include <yaml-cpp/yaml.h>

struct drea::core::Config::Private
{
	std::vector<std::string>		mFlags;
	std::vector<Option>				mOptions;

	std::optional<Option*> find( const std::string & flag ){
		std::optional<Option*>	res;

		for( Option & opt: mOptions ){
			if( opt.mName == flag ){
				res = &opt;
				break;
			}
		}
		return res;
	}

	void readConfig( int argc, char * argv[] )
	{
		std::filesystem::path		configFile;

		for( int i = 1; i < argc-1; i++ ){
			if( std::string( argv[i] ) == "--config-file" ){
				configFile = argv[i+1];
				break;
			}
		}
		if( !configFile.empty() && std::filesystem::exists( configFile ) ){
			YAML::Node config = YAML::LoadFile( configFile.string() );

			for( auto node: config ){
				std::string arg = node.first.as<std::string>();
				if( auto option = find( arg ) ){
					mFlags.push_back( arg );
					if( !option.value()->mParamName.empty() ){
						// discard data: config file and defaults
						option.value()->mValues.clear();

						if( node.second.Type() == YAML::NodeType::Scalar ){
							OptionValue	val = option.value()->fromString( node.second.as<std::string>() );

							if( val.index() > 0 ){
								option.value()->mValues.push_back( val );
							}else{
								exit( -1 );
							}
						}else if( node.second.Type() == YAML::NodeType::Sequence ){
							for( auto seqVal: node.second ){
								if( seqVal.Type() == YAML::NodeType::Scalar ){
									OptionValue	val = option.value()->fromString( seqVal.as<std::string>() );

									if( val.index() > 0 ){
										option.value()->mValues.push_back( val );
									}else{
										exit( -1 );
									}
								}
							}
						}
						if( option.value()->mValues.empty() ){
							spdlog::warn( "Missing arguments for flag {}", arg );
						}
					}
				}else{
					spdlog::warn( "Unknown argument {}", arg );
				}
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
			"log-file", "file name", "log messages to the file <file name>", {}, typeid( std::string )
		}
	).add(
		{
			"config-file", "file name", "read configs from file <file name>", {}, typeid( std::string )
		}
	);
}

drea::core::Config & drea::core::Config::add( drea::core::Option option )
{
	d->mOptions.push_back( option );
	return *this;
}

std::optional<drea::core::Option*> drea::core::Config::find( const std::string & flag ) const
{
	return d->find( flag );
}

std::vector<std::string> drea::core::Config::configure( int argc, char * argv[] )
{
	d->readConfig( argc, argv );

	std::vector<std::string>	others;

	for( int i = 1; i < argc; ){
		std::string  arg = argv[i++];

		if( arg.find( "--" ) == 0 ){
			arg = arg.erase( 0, 2 );
			
			if( auto option = d->find( arg ) ){
				d->mFlags.push_back( arg );
				if( !option.value()->mParamName.empty() ){
					// discard data: config file and defaults
					option.value()->mValues.clear();
					while( i < argc ){
						std::string subArg = argv[i];
						
						if( subArg.find( "-" ) == 0 ){
							break;
						}else{
							OptionValue	val = option.value()->fromString( subArg );

							if( val.index() > 0 ){
								option.value()->mValues.push_back( val );
								i++;
							}else{
								exit( -1 );
							}
						}
					}
					if( option.value()->mValues.empty() ){
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
		if( !option.mValues.empty() && !contains( option.mName )){
			d->mFlags.push_back( option.mName );
		}
	}
	return others;
}

bool drea::core::Config::contains( const std::string & flag ) const
{
	return std::find( d->mFlags.begin(), d->mFlags.end(), flag ) != d->mFlags.end();
}

void drea::core::Config::showHelp( int offset )
{
	for( const Option & option: d->mOptions ){
		for( int i = 0; i < offset; i++ ){
			std::cout << " ";
		}
		if( option.mParamName.empty() ){
			fmt::print( "[--{}]", option.mName );
		}else{
			fmt::print( "[--{} <{}>]", option.mName, option.mParamName );
		}
		fmt::print( " {}", option.mDescription );
		if( option.mValues.empty() ){
			fmt::print( "\n" );
		}else{
			fmt::print( ". Default" );
			for( auto v: option.mValues ){
				fmt::print( " {}", option.toString( v ));
			}
			fmt::print( "\n" );
		}
	}
	fmt::print( "\n" );
}
