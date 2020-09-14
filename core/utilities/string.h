#pragma once

#include <string>
#include <vector>

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

	for( const std::string & s: value ){
		if( res.empty() ){
			res = s;
		}else{
			res += delimiter + s;
		}
	}
	return res;
}

static std::string replace( const std::string & str, const std::string & from, const std::string & to )
{
	std::string	res = str;
	size_t start_pos = 0;
	while((start_pos = res.find(from, start_pos)) != std::string::npos) {
		res.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return res;
}

}}}}
