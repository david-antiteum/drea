# Drea

A C++ framework for CLI apps and services that includes support for:

- Commands (and subcommands)
- Options
- Logging

Drea tries to extract as much functionality with the minimum required developer intervention while preventing repeating information already present. For example, the developer declares commands and flags and, using that definition, Drea generates:

- Help
- Shell integration: Man pages and autocompletion
- Support for configuration options:
  - using files. Supported formats: TOML, YAML, JSON
  - shell variables
  - command flags
  - KV storers: consul and etcd
- User's input validation, including configuration files

## An example

This example has a single command ```this``` and one option ```reverse```. The configuration of the app is defined in a YAML file converted to a header file called ```commands.yml.h``` using the unix utility ```xxd```.

```yaml
app: say
version: 0.0.1
description: |
  Prints the argument of the command \"this\" and quits.
options:
  - option: reverse
    description: reverse string
commands:
  - command: this
    params-names: string
    description: prints the argument
    local-options:
      - reverse
```

```c++
#include <drea/core/Core>
#include <algorithm>

#include "commands.yml.h"

int main( int argc, char * argv[] )
{
    drea::core::App     app( argc, argv );

    app.config().addDefaults();
    app.commander().addDefaults();
    app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
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

An example of use:

```bash
λ ./say_this this hello
[2019-04-16 09:44:44.649] [say] [info] hello
```

Execute the example as ```./say_this --help``` to get help:

```text
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

We can also get additional help from a particular command, try this: ```./say_this this --help```.

## How to Build

Use CMake to build and install Drea in Linux, macOS and Windows systems.

## How to Use Drea

```CMake
find_package(DreaCore REQUIRED)

target_link_libraries(main PRIVATE DreaCore)
```

## Commands

Commands are the actions that your application can execute. A command can have subcommands, creating a hierarchy. When Drea requests the execution of a command, it will present the complete path, making it possible to have two subcommands with the same name.

### The anatomy of a command

1. A command can have arguments and options. Example:

```bash
./say this hello --reverse
```

The command is ```this```, has the argument ```hello``` and the option ```reverse```.

2. Any number of arguments

3. A command can have subcommands. Example:

```bash
./myapp container ls
```

The command is ```container``` and the subcommand ```ls```. Drea will ask for the execution of the command ```container.ls```. Can be defined as:

```c++
app.commander().addDefaults().add({
    {
        "container", "", "Manage containers"
    },
    {
        "config", "", "Manage configs"
    },
    {
        "ls", "", "List containers", {}, {}, "container"
    },
    {
        "ls", "", "List configs", {}, {}, "config"
    }
});
```

### Defining a command

A command has the following information:

- name: must be unique in a command path
- arguments: name of the arguments, if any
- description: the command description used by the help system
- local options: list of options that applies to this command
- global options: list of global options that applies to this and command
- parent command: the name of the parent command
- number of parameters: 0, 1,... unlimited

## Configuration

Use configuration options to modify the behaviour of commands and to set values for required parameters.

### Evaluation order

The order of evaluation, from lower to higher priority:

- defaults
- KV Store (as Consul or etcd)
- config file
- env variables
- command line flags
- explicit call to set

## Readings

- [On formats](https://news.ycombinator.com/item?id=19653834)
- [Terminology](https://pythonconquerstheuniverse.wordpress.com/2010/07/25/command-line-syntax-some-basic-concepts/): Command-line syntax: some basic concepts

### Meta configs

- [JSonnet](https://jsonnet.org/)
- [Dhall](https://dhall-lang.org/)

HN [discussion]( https://news.ycombinator.com/item?id=19656821 )

### Libs

- [Viper](https://github.com/spf13/viper): Viper is a complete configuration solution for Go applications including 12-Factor apps
- [Cobra](https://github.com/spf13/cobra): Cobra is both a library for creating powerful modern CLI applications as well as a program to generate applications and command files.
