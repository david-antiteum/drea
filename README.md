# Drea
A C++ framework for CLI apps and services

An example:

```c++
#include <drea/core/Core>
#include <algorithm>

int main( int argc, char * argv[] )
{
    drea::core::App     app( argc, argv );

    app.setName( "say" );
    app.setDescription( "Prints the argument of the command \"this\" and quits." );
    app.setVersion( "0.0.1" );

    app.config().addDefaults().add(
        {
            "reverse", "", "reverse string"
        }
    );
    app.commander().addDefaults().add(
        {
            "this", "string", "prints the argument", { "reverse" }
        }
    );
    app.parse();
    app.commander().run( [ &app ]( std::string cmd ){
        if( cmd == "this" && app.commander().arguments().size() == 1 ){
            std::string say = app.commander().arguments().front();
            if( app.config().used( "reverse" ) ){
                std::reverse( say.begin(), say.end() );
            }
            app.logger().info( "{}", say );
        }
    });
}
```

This example has a single command ```this``` and one option ```reverse```. An example of use:

```
λ ./say_this this hello
[2019-04-16 09:44:44.649] [say] [info] hello
```

Based on options and commands, Drea automatically adds some functionalities for us:

- Help
- Shell integration: Man pages and autocompletion
- Log: verbose level, log to file, log to remote server
- Support for configuration options using files, shell variables, command flags and KV storer

## Help

Execute the example as ```./say_this --help``` to get help:

```
Prints the argument of the command "this" and quits.

usage: say COMMAND [OPTIONS]

Options:
      --config-file file                 read configs from file <file>
      --graylog-host schema://host:port  Send logs to a graylog server. Example: http://localhost:12201
  -h, --help                             show help and quit
      --log-file file                    log messages to the file <file>
      --reverse                          reverse string
      --system-integration               generate man pages and scripts to enable autocompletion in the shell
  -v, --verbose                          increase the logging level to debug
      --version                          print version information and quit

Commands:
  this

Use "say COMMAND --help" for more information about a command.
```

We can also get additional help from a particular command. Try this: ```./say_this this --help```

# How to Build

Use CMake to build and install Drea in Linux, macOS and Windows systems.

# How to Use Drea

```
find_package(DreaCore REQUIRED)

target_link_libraries(main PRIVATE DreaCore)
```

# Configuration

Evaluation order:

- defaults
- config file
- env variables
- command line flags

# Readings

- [On formats](https://news.ycombinator.com/item?id=19653834)

## Meta configs

- [JSonnet](https://jsonnet.org/)
- [Dhall](https://dhall-lang.org/)

HN [discussion]( https://news.ycombinator.com/item?id=19656821 )

## Libs

- [Viper](https://github.com/spf13/viper): Viper is a complete configuration solution for Go applications including 12-Factor apps
- [Cobra](https://github.com/spf13/cobra): Cobra is both a library for creating powerful modern CLI applications as well as a program to generate applications and command files.

## Info

- [Terminology](https://pythonconquerstheuniverse.wordpress.com/2010/07/25/command-line-syntax-some-basic-concepts/): Command-line syntax: some basic concepts
