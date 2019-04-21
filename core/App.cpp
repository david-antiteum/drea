#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include "App.h"
#include "Config.h"
#include "Commander.h"

#include "utilities/parser.h"
#include <yaml-cpp/yaml.h>

struct drea::core::App::Private
{
	Private()
	{
	}

	std::string							mAppExeName;
	std::string							mDescription;
	std::string							mVersion = "0.0.0";
	Config								mConfig;
	Commander							mCommander;
	std::shared_ptr<spdlog::logger>		mLogger;
	std::vector<std::string>			mArgs;
};

static drea::core::App * mInstanceApp = nullptr;

drea::core::App::App( int argc, char * argv[] )
{
	d = std::make_unique<Private>();
	mInstanceApp = this;

	if( argc > 0 ){
		for( int i = 0; i < argc; i++ ){
			d->mArgs.push_back( argv[i] );
		}
		d->mAppExeName = argv[0];
	}
}

drea::core::App::~App()
{
	mInstanceApp = nullptr;
}

void _parseCmd( drea::core::App & app, const YAML::Node & cmdsNode, const std::string & parentId )
{
	if( cmdsNode.IsMap() ){
		drea::core::Command	command;

		command.mParentCommand = parentId;
		for( auto cmdNode: cmdsNode ){
			const std::string key = cmdNode.first.as<std::string>();
			if( cmdNode.second.IsScalar() ){
				if( key == "command" ){
					command.mName = cmdNode.second.as<std::string>();
				}else if( key == "description" ){
					command.mDescription = cmdNode.second.as<std::string>();
				}else if( key == "params-names" ){
					command.mParamName = cmdNode.second.as<std::string>();
				}else if( key == "params" ){
					if( cmdNode.second.as<std::string>() == "unlimited" ){
						command.mNbParams = drea::core::Command::mUnlimitedParams;
					}else{
						command.mNbParams = cmdNode.second.as<int>();
					}
				}
			}else if( cmdNode.second.IsSequence() ){
				if( key == "global-options" || key == "local-options" ){
					for( auto optionNode: cmdNode.second ){
						if( optionNode.IsScalar() ){
							if( key == "global-options" ){
								command.mGlobalParameters.push_back( optionNode.as<std::string>() );
							}else{
								command.mLocalParameters.push_back( optionNode.as<std::string>() );
							}
						}
					}
				}else if( key == "commands" ){
					for( auto subCmdsNode: cmdNode.second ){
						_parseCmd( app, subCmdsNode, parentId.empty() ? command.mName : parentId + "."  + command.mName );
					}
				}
			}
		}
		app.commander().add( command );
	}
}

void _parseOption( drea::core::App & app, const YAML::Node & optionsNode )
{
	if( optionsNode.IsMap()  ){
		drea::core::Option	option;
		for( auto optionNode: optionsNode ){
			const std::string key = optionNode.first.as<std::string>();
			if( optionNode.second.IsScalar() ){
				if( key == "option" ){
					option.mName = optionNode.second.as<std::string>();
				}else if( key == "description" ){
					option.mDescription = optionNode.second.as<std::string>();
				}else if( key == "params-names" ){
					option.mParamName = optionNode.second.as<std::string>();
				}else if( key == "params" ){
					if( optionNode.second.as<std::string>() == "unlimited" ){
						option.mNbParams = drea::core::Option::mUnlimitedParams;
					}else{
						option.mNbParams = optionNode.second.as<int>();
					}
				}else if( key == "short" ){
					option.mShortVersion = optionNode.second.as<int>();
				}else if( key == "type" ){
					const std::string & type = optionNode.second.as<std::string>();
					if( type == "bool" ){
						option.mType = typeid( bool );
					}else if( type == "int" ){
						option.mType = typeid( int );
					}else if( type == "double" ){
						option.mType = typeid( double );
					}else if( type == "string" ){
						option.mType = typeid( std::string );
					}
				}else if( key == "value" ){
					option.mValues.push_back( optionNode.second.as<std::string>() );
				}
			}else if( optionNode.second.IsSequence() ){
				if( key == "values" ){
					for( auto valueNode: optionNode.second ){
						if( valueNode.IsScalar() ){
							option.mValues.push_back( valueNode.as<std::string>() );
						}
					}
				}
			}
		}
		app.config().add( option );
	}
}

void drea::core::App::parse( const std::string & definitions )
{
	if( !definitions.empty() ){
		for( auto node: YAML::Load( definitions ) ){
			const std::string key = node.first.as<std::string>();
			if( key == "app" && node.second.IsScalar() ){
				setName( node.second.as<std::string>() );
			}else if( key == "version" && node.second.IsScalar() ){
				setVersion( node.second.as<std::string>() );
			}else if( key == "description" && node.second.IsScalar() ){
				setDescription( node.second.as<std::string>() );
			}else if( key == "options" && node.second.IsSequence() ){
				for( auto optionsNode: node.second ){
					_parseOption( *this, optionsNode );
				}
			}else if( key == "commands" ){
				for( auto cmdsNode: node.second ){
					_parseCmd( *this, cmdsNode, "" );
				}
			}
		}
	}
	auto args = utilities::Parser( *this, d->mArgs ).parse();
	config().configure( args.first );
	d->mLogger = config().setupLogger();
	commander().configure( args.second );
}

drea::core::App & drea::core::App::instance()
{
	return *mInstanceApp;
}

const std::string & drea::core::App::name() const
{
	return d->mAppExeName;
}

const std::string & drea::core::App::description() const
{
	return d->mDescription;
}

const std::string & drea::core::App::version() const
{
	return d->mVersion;
}

void drea::core::App::setName( const std::string & value )
{
	d->mAppExeName = value;
}

void drea::core::App::setDescription( const std::string & value )
{
	d->mDescription = value;
}

void drea::core::App::setVersion( const std::string & value )
{
	d->mVersion = value;
}

drea::core::Config & drea::core::App::config() const
{
	return d->mConfig;
}

drea::core::Commander & drea::core::App::commander() const
{
	return d->mCommander;
}

spdlog::logger & drea::core::App::logger() const
{
	if( d->mLogger ){
		return *d->mLogger;
	}else{
		return *spdlog::default_logger();
	}
}

std::vector<std::string> drea::core::App::args() const
{
	return d->mArgs;
}
