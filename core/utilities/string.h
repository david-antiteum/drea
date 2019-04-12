#pragma once

#include <string>
#include <vector>
#include <boost/algorithm/string/replace.hpp>

namespace drea { namespace core { namespace utilities { namespace string {

static std::vector<std::string> split( const std::string & s, const std::string & delimiter )
{
	std::vector<std::string>	res;
	size_t 						last = 0; 
	size_t 						next = 0; 
	
	while(( next = s.find( delimiter, last )) != std::string::npos ){
		res.push_back( s.substr( last, next-last ));
		last = next + 1;
	}
	res.push_back( s.substr( last ));

	return res;
}

static std::string join( const std::vector<std::string> & value, const std::string & delimiter )
{
	std::string		res;

	for( auto s: value ){
		if( res.empty() ){
			res = s;
		}else{
			res += delimiter + s;
		}
	}
	return res;
}

static std::string replace( const std::string & s, const std::string & from, const std::string & to )
{
	return boost::replace_all_copy( s, from, to );
}

}}}}
