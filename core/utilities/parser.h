#pragma once

#include <string>
#include <vector>
#include <spdlog/fmt/fmt.h>
#include <iostream>

#include "App.h"

#include "utilities/string.h"

namespace drea::core::utilities { 
	
class Parser 
{
public:
	explicit Parser( App & app, const std::vector<std::string> & args ) : mApp( app ), mArgs( args ){}

	[[nodiscard]] std::vector<std::string> expand() const
	{
		std::vector<std::string>	res;

		// TODO expand args (for example -v to --verbose or -zvf <filename> to --compress --verbose --file <filename)
		for( size_t i = 1; i < mArgs.size(); i++ ){
			if( std::string arg = mArgs.at(i); arg.find( "--" ) == 0 ){
				res.push_back( arg );
			}else if( arg.find( "-" ) == 0 ){
				// Expand
				for( size_t j = 1; j < arg.size(); j++ ){
					std::string mini;
					mini.push_back( arg.at( j ) );
					if( auto option = mApp.config().find( mini )){
						res.push_back( fmt::format( "--{}", option->mName ) );
					}else{
						mApp.logger().warn( "Unknown short option -{}", arg.at( j ) );
					}
				}
			}else{
				res.push_back( arg );
			}
		}
		return res;
	}

	[[nodiscard]] std::pair<std::vector<std::string>,std::vector<std::string>> parse() const
	{
		std::vector<std::string>	args;
		std::vector<std::string>	cmds;
	
		auto expandedArgs = expand();
		for( size_t i = 0; i < expandedArgs.size(); ){
			if( std::string arg = expandedArgs.at( i++ ); arg.find( "--" ) == 0 ){
				args.push_back( arg );
				arg.erase( 0, 2 );
				if( auto option = mApp.config().find( arg ) ){
					for( int np = 0; np < option->numberOfParams() && i < expandedArgs.size(); np++ ){
						std::string subArg = expandedArgs.at( i );
						if( subArg.find( "-" ) == 0 ){
							break;
						}else{
							args.push_back( subArg );
							i++;
						}
					}
				}
			}else{
				cmds.push_back( arg );
			}
		}
		return { args, cmds };
	}

private:
	App								& mApp;
	const std::vector<std::string> 	& mArgs;
};

}
