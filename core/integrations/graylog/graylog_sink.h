#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/fmt/fmt.h>
#include <chrono>

#undef U
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

			utilities::httpclient::post( mGraylogService, logJSON );
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

std::shared_ptr<spdlog::logger> newLogger( const std::string & appName, bool verbose, const std::string & logFile, const std::string & graylogHost )
{
	std::shared_ptr<spdlog::logger> 	res;
	std::vector<spdlog::sink_ptr> 		sinks;

	sinks.push_back( std::make_shared<spdlog::sinks::stdout_color_sink_st>() );
	if( !logFile.empty() ){
		sinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>( logFile, 1048576 * 5, 3 ) );
	}
	if( !graylogHost.empty() ){
		sinks.push_back( std::make_shared<graylog_sink<spdlog::details::null_mutex>>( appName, graylogHost ) );
	}
	res = std::make_shared<spdlog::logger>( appName, sinks.begin(), sinks.end() );

	if( verbose ){
		res->set_level( spdlog::level::debug );
	}
	return res;
}

}}}}
