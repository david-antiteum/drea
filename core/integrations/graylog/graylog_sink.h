#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/fmt/fmt.h>
#include <chrono>

#include "utilities/httpclient.h"

namespace drea { namespace core { namespace integrations { namespace logs {

// example
// auto sink = std::make_shared<drea::core::integrations::logs::graylog_sink<spdlog::details::null_mutex>>( appName, graylogHost );

template<typename Mutex>
class graylog_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
	graylog_sink( const std::string & hostName, const std::string & graylogHost ) 
		: mHostName( hostName )
	{
		mGraylogService = fmt::format( "{}/gelf", graylogHost );
	}

protected:
	void sink_it_(const spdlog::details::log_msg& msg) override
	{
		if( msg.level != spdlog::level::level_enum::off ){
			nlohmann::json		logJSON;

			logJSON["version"] = "1.1";
			logJSON["host"] = mHostName;
			logJSON["short_message"] = fmt::format( msg.payload );
			logJSON["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>( msg.time.time_since_epoch() ).count() / 1000.0;
			logJSON["level"] = toLevel( msg.level );

			utilities::HttpClient::post( mGraylogService, logJSON );
		}
	}

	void flush_() override 
	{
	}

private:
	std::string			mHostName;
	std::string			mGraylogService;

	int toLevel( spdlog::level::level_enum level ){
		int grayLevel = 1;

		switch( level )
		{
			case spdlog::level::level_enum::trace :
				grayLevel = 7;
			break;

			case spdlog::level::level_enum::debug :
				grayLevel = 7;
			break;

			case spdlog::level::level_enum::info :
				grayLevel = 6;
			break;

			case spdlog::level::level_enum::warn :
				grayLevel = 4;
			break;

			case spdlog::level::level_enum::err :
				grayLevel = 3;
			break;

			case spdlog::level::level_enum::critical :
				grayLevel = 2;
			break;

			default:
			break;			
		}
		return grayLevel;
	}
};

}}}}
