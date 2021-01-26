#pragma once

#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <uriparser/Uri.h>

namespace drea::core::utilities {

[[nodiscard]] static std::optional<unsigned short> asPort( const std::string & portText )
{
	std::optional<unsigned short>	res;

	try{
		if( int resMaybe = std::stoi( portText ); resMaybe >= 0 && resMaybe <= std::numeric_limits<unsigned short>::max() ){
			res = static_cast<unsigned short>( resMaybe );
		}else{
			spdlog::error( "Port: Out of range" );
		}
	}catch( const std::invalid_argument & ia ){
		spdlog::error( "Port: Invalid argument {}", ia.what() );
	}catch( const std::out_of_range & oor ){
		spdlog::error( "Port: Out of range {}", oor.what() );
	}
	return res;
}

// https://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform

class Uri //: boost::noncopyable
{
public:
	explicit Uri( std::string_view uri )
		: uri_(uri)
	{
		UriParserStateA state_;
		state_.uri = &uriParse_;
		isValid_   = uriParseUriA(&state_, uri_.c_str()) == URI_SUCCESS;
	}

	Uri() = delete;
	Uri( Uri & other ) = delete;
	Uri( Uri && other ) = delete;
	const Uri & operator=( Uri && other ) = delete;

	~Uri() { uriFreeUriMembersA(&uriParse_); }

	[[nodiscard]] bool isValid() const
	{
		return isValid_ && asPort( fromRange(uriParse_.portText) );
	}

	[[nodiscard]] std::string scheme() const
	{
		return fromRange(uriParse_.scheme);
	}

	[[nodiscard]] std::string host() const
	{
		std::string res = fromRange(uriParse_.hostText);
		if( res == "localhost" ){
			res = "127.0.0.1";
		}
		return res;
	}

	[[nodiscard]] unsigned short port() const
	{
		if( auto res = asPort( fromRange(uriParse_.portText) ); res ){
			return res.value();
		}
		return 0;
	}

	[[nodiscard]] std::string path()     const { return fromList(uriParse_.pathHead, "/"); }
	[[nodiscard]] std::string query()    const { return fromRange(uriParse_.query); }
	[[nodiscard]] std::string fragment() const { return fromRange(uriParse_.fragment); }

private:
	std::string uri_;
	UriUriA     uriParse_;
	bool        isValid_;

	[[nodiscard]] std::string fromRange(const UriTextRangeA & rng) const
	{
		return std::string(rng.first, rng.afterLast);
	}

	[[nodiscard]] std::string fromList(UriPathSegmentA * xs, const std::string & delim) const
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

}
