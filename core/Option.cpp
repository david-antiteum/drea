#include "Option.h"

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

drea::core::OptionValue drea::core::Option::fromString( const std::string & val ) const
{
	OptionValue		res = std::monostate();

	if( mType == typeid( bool )){
		res = val == "yes" || val == "true";
	}else if( mType == typeid( int ) ){
		try{
			res = std::stoi( val );
		}catch(...){
			spdlog::critical( "Incorrect argument type for option {}. Must be an integer number", mName );
		}
	}else if( mType == typeid( double ) ){
		try{
			res = std::stod( val );
		}catch(...){
			spdlog::critical( "Incorrect argument type for option {}. Must be an floating number", mName );
		}
	}else if( mType == typeid( std::string ) ){
		res = val;
	}else{
		spdlog::critical( "Incorrect argument type for option {}", mName );
	}
	return res;
}
