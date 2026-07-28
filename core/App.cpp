#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>

#include "App.h"
#include "Config.h"
#include "Commander.h"
#include <drea/core/ExitCode.h>

#include "utilities/parser.h"
#include <yaml-cpp/yaml.h>

struct drea::core::App::Private
{
	explicit Private( App & app ) : mConfig( app ), mCommander( app )
	{
	}

	std::string							mAppExeName;
	std::string							mDescription;
	std::string							mVersion = "0.0.0";
	Config								mConfig;
	Commander							mCommander;
	std::shared_ptr<spdlog::logger>		mLogger;
	drea::log::Logger					mLog{ *spdlog::default_logger() };
	std::vector<std::string>			mArgs;
	std::string							mDefinitions;
	HelpFooterFn						mHelpFooter;
};

static drea::core::App * mInstanceApp = nullptr;

drea::core::App::App( int argc, char * argv[] ) : d( std::make_unique<Private>( *this ) )
{
	if( mInstanceApp == nullptr ){
		// allow two Apps for internal uses
		mInstanceApp = this;
	}
	if( argc > 0 ){
		for( int i = 0; i < argc; i++ ){
			d->mArgs.push_back( argv[i] );
		}
		d->mAppExeName = argv[0];
	}
}

drea::core::App::~App()
{
	if( mInstanceApp == this ){
		mInstanceApp = nullptr;
	}
}

/*! The `root:` block: the positional arguments the app takes when no command
	is given. Same keys as a command, minus the ones that only make sense for a
	named command (command, commands, group, options).
*/
void _parseRoot( drea::core::App & app, const YAML::Node & rootNode )
{
	if( rootNode.IsMap() ){
		drea::core::Command	root;

		root.mParamName = "arguments";
		for( auto node: rootNode ){
			const std::string key = node.first.as<std::string>();
			if( node.second.IsScalar() ){
				if( key == "params-names" ){
					root.mParamName = node.second.as<std::string>();
				}else if( key == "params" ){
					if( node.second.as<std::string>() == "unlimited" ){
						root.mNbParams = drea::core::Command::mUnlimitedParams;
					}else{
						root.mNbParams = node.second.as<int>();
					}
				}else if( key == "min-params" ){
					root.mMinParams = node.second.as<int>();
				}else if( key == "description" ){
					root.mDescription = node.second.as<std::string>();
				}
			}else if( node.second.IsSequence() ){
				if( key == "examples" ){
					for( auto exampleNode: node.second ){
						if( exampleNode.IsScalar() ){
							root.mExamples.push_back( exampleNode.as<std::string>() );
						}
					}
				}else if( key == "param-choices" ){
					for( auto choiceNode: node.second ){
						if( choiceNode.IsScalar() ){
							root.mParamChoices.push_back( choiceNode.as<std::string>() );
						}
					}
				}
			}
		}
		app.commander().setRoot( root );
	}
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
				}else if( key == "min-params" ){
					command.mMinParams = cmdNode.second.as<int>();
				}else if( key == "deprecated" ){
					command.mDeprecated = cmdNode.second.as<bool>();
				}else if( key == "group" ){
					command.mGroups.push_back( cmdNode.second.as<std::string>() );
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
				}else if( key == "group" ){
					for( auto groupNode: cmdNode.second ){
						if( groupNode.IsScalar() ){
							command.mGroups.push_back( groupNode.as<std::string>() );
						}
					}
				}else if( key == "examples" ){
					for( auto exampleNode: cmdNode.second ){
						if( exampleNode.IsScalar() ){
							command.mExamples.push_back( exampleNode.as<std::string>() );
						}
					}
				}else if( key == "param-choices" ){
					for( auto choiceNode: cmdNode.second ){
						if( choiceNode.IsScalar() ){
							command.mParamChoices.push_back( choiceNode.as<std::string>() );
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
		drea::core::Option			option;
		bool						hasType = false;
		std::vector<std::string>	rawValues;	//!< value:/values: entries, converted once the declared type is known

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
				}else if( key == "scope" ){
					if( optionNode.second.as<std::string>() == "both" ){
						option.mScope = drea::core::Option::Scope::Both;
					}else if( optionNode.second.as<std::string>() == "file" ){
						option.mScope = drea::core::Option::Scope::File;
					}else if( optionNode.second.as<std::string>() == "line" ){
						option.mScope = drea::core::Option::Scope::Line;
					}else if( optionNode.second.as<std::string>() == "none" ){
						option.mScope = drea::core::Option::Scope::None;
					}
				}else if( key == "short" ){
					option.mShortVersion = optionNode.second.as<std::string>();
				}else if( key == "sensitive" ){
					option.mSensitive = optionNode.second.as<bool>();
				}else if( key == "required" ){
					option.mRequired = optionNode.second.as<bool>();
				}else if( key == "deprecated" ){
					option.mDeprecated = optionNode.second.as<bool>();
				}else if( key == "negatable" ){
					option.mNegatable = optionNode.second.as<bool>();
				}else if( key == "min" ){
					option.mMin = optionNode.second.as<double>();
				}else if( key == "max" ){
					option.mMax = optionNode.second.as<double>();
				}else if( key == "type" ){
					if( const std::string & type = optionNode.second.as<std::string>(); type == "bool" ){
						option.mType = typeid( bool );
					}else if( type == "int" ){
						option.mType = typeid( int );
					}else if( type == "double" ){
						option.mType = typeid( double );
					}else if( type == "string" ){
						option.mType = typeid( std::string );
					}else{
						// fatal, wrong type
						app.logger().critical( "Wrong option type {} for {}", type, option.mName );
						exit( drea::core::toInt( drea::core::ExitCode::ConfigError ) );
					}
					hasType = true;
				}else if( key == "value" ){
					// kept as text like values:, so a mapping stays what YAML
					// says it is, unordered: this used to be converted here and
					// died when it came before type:
					if( std::string declared = optionNode.second.as<std::string>(); declared.empty() ){
						app.logger().critical( "Empty value for option {}", option.mName );
						exit( drea::core::toInt( drea::core::ExitCode::ConfigError ) );
					}else{
						rawValues.push_back( declared );
					}
				}
			}else if( optionNode.second.IsSequence() ){
				if( key == "values" ){
					// kept as text for now: the type may be declared after the
					// values, and it decides how each default is stored
					for( auto valueNode: optionNode.second ){
						if( valueNode.IsScalar() ){
							rawValues.push_back( valueNode.as<std::string>() );
						}
					}
				}else if( key == "choices" ){
					for( auto choiceNode: optionNode.second ){
						if( choiceNode.IsScalar() ){
							option.mChoices.push_back( choiceNode.as<std::string>() );
						}
					}
				}
			}
		}
		// An option that takes no value is a toggle. Left to the default type
		// it would be a string that can never hold anything: --opt would only
		// register the use, --no-opt would not apply, and a value coming from
		// a config file or the environment would be ignored. Only when the
		// author declared no type at all, so an explicit "type: string" is
		// respected. scope: none is exempt: the app sets those in code, with a
		// value of its own. choices are exempt too: bool has a closed domain
		// already, and inferring would turn the declaration into a finding.
		if( !hasType && option.mParamName.empty() && option.mChoices.empty()
			&& option.mValues.empty() && rawValues.empty()
			&& option.mScope != drea::core::Option::Scope::None ){
			option.mType = typeid( bool );
		}
		// The declared type decides how a default is stored: a string kept in
		// an int option would throw later, through Config::get<int> or
		// Option::toString. Same conversion as every other source.
		for( const std::string & rawValue: rawValues ){
			if( auto value = option.fromString( rawValue ); value.index() > 0 ){
				option.mValues.push_back( value );
			}else{
				app.logger().critical( "Default value \"{}\" declared for option {} is not a valid {}",
					rawValue, option.mName, option.typeName() );
				exit( drea::core::toInt( drea::core::ExitCode::ConfigError ) );
			}
		}
		app.config().add( option );
	}
}

void drea::core::App::addToParser( std::string_view definitions )
{
	if( !d->mDefinitions.empty() ){
		d->mDefinitions += "\n";
	}
	d->mDefinitions += definitions;
}

void drea::core::App::parse()
{
	parse( d->mDefinitions );
}

void drea::core::App::parse( const std::string & definitions )
{
	// TODO.. disable defaults from definitions
	bool		useConfigDefaults = true;
	bool		useCommanderDefaults = true;

	if( !definitions.empty() ){
		for( auto node: YAML::Load( definitions ) ){
			if( const std::string key = node.first.as<std::string>(); key == "app" && node.second.IsScalar() ){
				setName( node.second.as<std::string>() );
			}else if( key == "version" && node.second.IsScalar() ){
				setVersion( node.second.as<std::string>() );
			}else if( key == "description" && node.second.IsScalar() ){
				setDescription( node.second.as<std::string>() );
			}else if( key == "env-prefix" && node.second.IsScalar() ){
				config().setEnvPrefix( node.second.as<std::string>() );
			}else if( key == "options" && node.second.IsSequence() ){
				for( auto optionsNode: node.second ){
					_parseOption( *this, optionsNode );
				}
			}else if( key == "commands" ){
				for( auto cmdsNode: node.second ){
					_parseCmd( *this, cmdsNode, "" );
				}
			}else if( key == "root" && node.second.IsMap() ){
				_parseRoot( *this, node.second );
			}
		}
	}
	if( useConfigDefaults ){
		config().addDefaults();
	}
	if( useCommanderDefaults ){
		commander().addDefaults();
	}
	// In validate mode the report is the only output, so nothing may log before
	// it: the argument parsing itself warns (an unknown short option, for one)
	// and the console sinks write to stdout, where --validate --json must emit
	// clean JSON. Detected on the raw arguments for that reason, ahead of every
	// other step.
	const bool	validateMode = config().find( "validate" )
		&& std::find( d->mArgs.begin(), d->mArgs.end(), "--validate" ) != d->mArgs.end();

	if( validateMode ){
		spdlog::default_logger()->set_level( spdlog::level::off );
	}
	auto args = utilities::Parser( *this, d->mArgs ).parse();

	configureInRunTime();
	config().configure( args.first );
	d->mLogger = config().setupLogger();
	if( validateMode ){
		d->mLogger->set_level( spdlog::level::off );
	}
	d->mLog.reset( *d->mLogger );
	// the describe builtin describes the CLI, not the configuration: like
	// --help it must work when the config is broken. An app that defines a
	// command of its own named describe does not get the exemption.
	const bool describeMode = [ this, &args ]{
		if( args.second.empty() || args.second.at( 0 ) != "describe" ){
			return false;
		}
		auto cmd = commander().find( "describe" );
		return cmd && cmd->mPredefined;
	}();
	// --help, --version, describe and --validate must work even when
	// the config is invalid: --validate reports the problems itself
	if( !config().used( "help" ) && !config().used( "version" ) && !describeMode && !config().used( "validate" ) ){
		if( const auto errors = config().validate(); !errors.empty() ){
			for( const auto & error: errors ){
				logger().critical( "{}", error );
			}
			d->mLogger->flush();
			exit( toInt( ExitCode::ConfigError ) );
		}
		if( config().get<bool>( "log-effective-config" ) ){
			config().logEffective( logger() );
		}
	}
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

void drea::core::App::setName( std::string_view value )
{
	d->mAppExeName = value;
}

void drea::core::App::setDescription( std::string_view value )
{
	d->mDescription = value;
}

void drea::core::App::setVersion( std::string_view value )
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

drea::log::Logger & drea::core::App::logger() const
{
	// once parse has installed the configured logger this is a pure read,
	// safe for concurrent callers; before that (single-threaded startup)
	// follow the spdlog default logger, which the user may still swap
	if( !d->mLogger ){
		d->mLog.reset( *spdlog::default_logger() );
	}
	return d->mLog;
}

std::vector<std::string> drea::core::App::args() const
{
	return d->mArgs;
}

void drea::core::App::setHelpFooter( drea::core::HelpFooterFn fn )
{
	d->mHelpFooter = std::move( fn );
}

std::string drea::core::App::helpFooter( std::string_view command ) const
{
	if( d->mHelpFooter ){
		return d->mHelpFooter( *this, command );
	}
	return {};
}
