#pragma once

#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <uriparser/Uri.h>

namespace drea { namespace core { namespace utilities {

// https://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform

class Uri //: boost::noncopyable
{
public:
	Uri( const std::string & uri )
		: uri_(uri)
	{
		UriParserStateA state_;
		state_.uri = &uriParse_;
		isValid_   = uriParseUriA(&state_, uri_.c_str()) == URI_SUCCESS;
	}

	~Uri() { uriFreeUriMembersA(&uriParse_); }

	bool isValid() const { return isValid_ && port() >= 0; }

	std::string scheme()   const { return fromRange(uriParse_.scheme); }
	std::string host()     const
	{
		std::string res = fromRange(uriParse_.hostText);
		if( res == "localhost" ){
			res = "127.0.0.1";
		}
		return res;
	}

	int port() const
	{
		int	res = -1;

		try{
			res = std::stoi( fromRange(uriParse_.portText) ); 
		}catch( ... ){

		}
		return res;
	}

	std::string path()     const { return fromList(uriParse_.pathHead, "/"); }
	std::string query()    const { return fromRange(uriParse_.query); }
	std::string fragment() const { return fromRange(uriParse_.fragment); }

private:
	std::string uri_;
	UriUriA     uriParse_;
	bool        isValid_;

	std::string fromRange(const UriTextRangeA & rng) const
	{
		return std::string(rng.first, rng.afterLast);
	}

	std::string fromList(UriPathSegmentA * xs, const std::string & delim) const
	{
		UriPathSegmentStructA * head(xs);
		std::string accum;

		while (head)
		{
			accum += delim + fromRange(head->text);
			head = head->next;
		}

		return accum;
	}
};

}}}
