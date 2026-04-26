#pragma once

#include <cctype>
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
			}else if( arg.size() >= 2 && arg.at( 0 ) == '-' && ( std::isdigit( static_cast<unsigned char>( arg.at( 1 ) ) ) || arg.at( 1 ) == '.' ) ){
				// Negative number / numeric positional: pass through unchanged.
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
				std::string nameOnly = arg.substr( 2 );
				bool hasInlineValue = false;
				if( auto eq = nameOnly.find( '=' ); eq != std::string::npos ){
					nameOnly = nameOnly.substr( 0, eq );
					hasInlineValue = true;
				}
				if( auto option = mApp.config().find( nameOnly ); option && !hasInlineValue ){
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
