#include "Option.h"

#include <cmath>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

std::string drea::core::Option::toString( const OptionValue & val ) const
{
	std::string		res;

	if( mType == typeid( bool )){
		res = fmt::format( "{}", std::get<bool>( val ) );
	}else if( mType == typeid( int ) ){
		res = fmt::format( "{}", std::get<int>( val ) );
	}else if( mType == typeid( double ) ){
		res = fmt::format( "{}", std::get<double>( val ) );
	}else if( mType == typeid( std::string ) ){
		res = std::get<std::string>( val );
	}
	return res;
}

std::string drea::core::Option::typeName() const
{
	if( mType == typeid( bool ) ){
		return "bool";
	}else if( mType == typeid( int ) ){
		return "int";
	}else if( mType == typeid( double ) ){
		return "double";
	}
	return "string";
}

std::string drea::core::Option::scopeName() const
{
	switch( mScope ){
		case Scope::File:	return "config-file";
		case Scope::Line:	return "command-line";
		case Scope::None:	return "none";
		default:			return "both";
	}
}

drea::core::OptionValue drea::core::Option::fromString( const std::string & val ) const
{
	OptionValue		res = std::monostate();

	if( mType == typeid( bool )){
		// closed vocabulary: anything else is a type error, not false
		if( val == "true" || val == "yes" || val == "1" ){
			res = true;
		}else if( val == "false" || val == "no" || val == "0" ){
			res = false;
		}else{
			spdlog::critical( "Incorrect argument type for option {}: \"{}\" is not a boolean. Use true/false, yes/no or 1/0", mName, val );
		}
	}else if( mType == typeid( int ) ){
		try{
			size_t	consumed = 0;
			int		parsed = std::stoi( val, &consumed );

			// reject partial parses like "80x"
			if( consumed == val.size() ){
				res = parsed;
			}else{
				spdlog::critical( "Incorrect argument type for option {}: \"{}\". Must be an integer number", mName, val );
			}
		}catch( const std::exception & e ){
			spdlog::critical( "Incorrect argument type for option {}: {}. Must be an integer number", mName, e.what() );
		}
	}else if( mType == typeid( double ) ){
		try{
			size_t	consumed = 0;
			double	parsed = std::stod( val, &consumed );

			if( consumed != val.size() ){
				spdlog::critical( "Incorrect argument type for option {}: \"{}\". Must be an floating number", mName, val );
			}else if( !std::isfinite( parsed ) ){
				// std::stod accepts "nan" and "inf": neither can be compared
				// against min/max, and neither is representable in JSON
				spdlog::critical( "Incorrect argument type for option {}: \"{}\". Must be a finite floating number", mName, val );
			}else{
				res = parsed;
			}
		}catch( const std::exception & e ){
			spdlog::critical( "Incorrect argument type for option {}: {}. Must be an floating number", mName, e.what() );
		}
	}else if( mType == typeid( std::string ) ){
		res = val;
	}else{
		spdlog::critical( "Incorrect argument type for option {}", mName );
	}
	return res;
}
