# drea
A C++ framework for CLI apps and services

An example:

```
#include <core/Core>
#include <algorithm>

int main( int argc, char * argv[] )
{
	drea::core::App	 app;

	app.setName( "say" );
	app.setDescription( "An example for the Drea Framework.\n\nDrea is available at https://github.com/david-antiteum/drea." );
	app.setVersion( "0.0.1" );
	
	app.config().addDefaults().add(
		{
			"reverse", "", "reverse string"
		}
	);
	app.commander().addDefaults().add(
		{
			"say", "prints the argument", {}, { "reverse" }
		}
	);
	app.parse( argc, argv );
	app.commander().run( [ &app ]( std::string cmd ){
		if( cmd == "say" && app.commander().arguments().size() == 1 ){
			std::string say = app.commander().arguments().front();
			if( app.config().contains( "reverse" ) ){
				std::reverse( say.begin(), say.end() );
			}
			app.logger()->info( "{}", say );
		}
	});
}
```

## Configuration

Evaluation order:
- defaults
- config file
- env variables
- command line flags

## Readings

### Libs

- [Viper](https://github.com/spf13/viper): Viper is a complete configuration solution for Go applications including 12-Factor apps
- [Cobra](https://github.com/spf13/cobra): Cobra is both a library for creating powerful modern CLI applications as well as a program to generate applications and command files.

### Info

- [Terminology](
https://pythonconquerstheuniverse.wordpress.com/2010/07/25/command-line-syntax-some-basic-concepts/
): Command-line syntax: some basic concepts

