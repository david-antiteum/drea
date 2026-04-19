#pragma once

#ifdef ENABLE_AWS

#include <mutex>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <aws/core/Aws.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/secretsmanager/SecretsManagerClient.h>
#include <aws/secretsmanager/model/GetSecretValueRequest.h>

namespace drea::core::integrations::aws {

class SdkLifecycle
{
public:
	static void ensureInitialized()
	{
		static SdkLifecycle instance;
		(void)instance;
	}

private:
	SdkLifecycle()
	{
		Aws::InitAPI( mOptions );
	}
	~SdkLifecycle()
	{
		Aws::ShutdownAPI( mOptions );
	}
	SdkLifecycle( const SdkLifecycle & ) = delete;
	SdkLifecycle & operator=( const SdkLifecycle & ) = delete;

	Aws::SDKOptions mOptions;
};

/*! Access a secret in AWS Secrets Manager using the local IAM principal.
	The secret string is returned verbatim; drea's config layer parses it as
	JSON/TOML/YAML just like any other remote source.
*/
class SecretsManager
{
public:
	explicit SecretsManager( const std::string & region )
		: mRegion( region )
	{
	}

	std::string get( const std::string & secretId ) const
	{
		std::string res;

		spdlog::info( "accesing to AWS Secrets Manager for secret '{}' in region '{}'", secretId, mRegion );

		SdkLifecycle::ensureInitialized();

		Aws::Client::ClientConfiguration clientConfig;
		if( !mRegion.empty() ){
			clientConfig.region = mRegion;
		}

		Aws::SecretsManager::SecretsManagerClient client( clientConfig );
		Aws::SecretsManager::Model::GetSecretValueRequest request;
		request.SetSecretId( secretId );

		auto outcome = client.GetSecretValue( request );
		if( outcome.IsSuccess() ){
			const auto & result = outcome.GetResult();
			if( !result.GetSecretString().empty() ){
				res = result.GetSecretString();
			}else{
				spdlog::warn( "AWS Secrets Manager returned a binary secret for '{}'; binary secrets are not supported", secretId );
			}
		}else{
			spdlog::error( "AWS Secrets Manager error for secret '{}': {}", secretId, outcome.GetError().GetMessage() );
		}
		return res;
	}

private:
	std::string mRegion;
};

}

#endif
