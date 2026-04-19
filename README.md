# Drea

A C++ framework for CLI apps and services that includes support for:

- Commands (and subcommands)
- Options
- Logging

Drea aims to give you maximum functionality with minimum ceremony. Declare your commands and options once; Drea derives the rest:

- Help output
- Shell integration: man pages and autocompletion
- Configuration from multiple sources:
  - files (TOML, YAML, JSON)
  - environment variables
  - command-line flags
  - remote sources via `--config-source` (currently AWS Secrets Manager)
- Input validation, including automatic argument-count checks and configuration files

## An example

This example has a single command `this` and one option `reverse`. The configuration of the app is defined in a YAML file converted to a header file called `commands.yml.h` using the Unix utility `xxd` (Windows users: [Vim](https://www.vim.org/download.php) ships a copy).

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

    app.parse( std::string( commands_yml, commands_yml + commands_yml_len ) );
    app.commander().run( [ &app ]( const std::string & cmd ){
        if( cmd == "this" ){
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
λ ./saythis this hello
[2019-04-16 09:44:44.649] [say] [info] hello
```

Execute the example as `./saythis --help` to get help:

```text
Prints the argument of the command "this" and quits.

usage: say COMMAND [OPTIONS]

Options:
      --config-file file                  read configs from file <file>
      --graylog-host schema://host:port   Send logs to a graylog server. Example: http://localhost:12201
  -h, --help                              show help and quit
      --log-file file                     log messages to the file <file>
      --log-folder folder                 log messages to file say.log in <folder>
      --log-nb-files number-of-log-files  <number-of-log-files> to keep. Default 10
      --log-size size                     log <size> (in MB) for each log file. Default 10
  -v, --verbose                           increase the logging level to debug
  -V, --version                           print version information and quit

Commands:
  this  prints the argument

Use "say COMMAND --help" for more information about a command.
```

Note that `--reverse` is not listed in the global help: options declared only as `local-options` of a command are shown only where they apply. Try `./saythis this --help` to see them.

## How to Build

Drea ships CMake presets (requires CMake >= 3.21 and Ninja). Point `VCPKG_ROOT`
at your vcpkg checkout and pick a preset:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Available configure presets:

| Preset           | Build type      | Examples | Tests | Toolchain      |
|------------------|-----------------|----------|-------|----------------|
| `debug`          | Debug           | yes      | yes   | vcpkg          |
| `release`        | RelWithDebInfo  | yes      | yes   | vcpkg          |
| `sdk`            | Release         | no       | no    | vcpkg          |
| `debug-system`   | Debug           | yes      | yes   | system libs    |
| `release-system` | RelWithDebInfo  | yes      | yes   | system libs    |
| `sdk-system`     | Release         | no       | no    | system libs    |

Opt-in vcpkg features (add to the configure call):

```bash
cmake --preset debug -DVCPKG_MANIFEST_FEATURES=toml        # TOML config files
cmake --preset debug -DVCPKG_MANIFEST_FEATURES=aws         # AWS Secrets Manager
cmake --preset debug -DVCPKG_MANIFEST_FEATURES="toml;aws"  # both
```

To enable the AWS Secrets Manager source in code, also pass `-DENABLE_AWS=ON`.

If you prefer a bare configure (no preset), the usual form still works:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## How to Use Drea

```CMake
find_package(drea REQUIRED)
target_link_libraries(main PRIVATE drea)
```

## Commands

Commands are the actions that your application can execute. A command can have subcommands, creating a hierarchy. When Drea invokes the run callback, it passes the full command path (for example, `container.ls`), so two subcommands under different parents can share the same name.

### The anatomy of a command

1. A command can have arguments and options. Example:

```bash
./say this hello --reverse
```

The command is `this`, with argument `hello` and option `reverse`.

2. A command can take a fixed number of arguments, an unlimited number (`params: unlimited`), or a range where some are optional (see `min-params` below).

3. A command can have subcommands. Example:

```bash
./myapp container ls
```

The command is `container` and the subcommand is `ls`. Drea will invoke the callback with the full path `container.ls`. It can be defined as:

```yaml
commands:
  - command: container
    description: Manage containers
    commands:
    - command: ls
      description: List containers
  - command: config
    description: Manage configs
    commands:
    - command: ls
      description: List configs
```

### Defining a command

A command has the following information:

- `command`: name, must be unique in a command path
- `params-names`: names of the positional arguments, if any
- `description`: the command description used by the help system
- `local-options`: options that apply only to this command
- `global-options`: options that apply to this command and its subcommands
- `parent`: the name of the parent command (set automatically when nested under `commands:`)
- `params`: number of positional arguments: `0`, `1`, ..., or `unlimited`
- `min-params` (optional): minimum number of positional arguments when some are optional. If set, `params` is the maximum and `min-params` the minimum.

When `run()` is invoked, Drea validates the argument count against `params` (and `min-params` if set) and reports an error without calling the callback if they do not match.

### Hiding commands at runtime

Set `Command::mHidden = true` to exclude a command from `--help`, bash autocompletion, and the "did you mean?" suggestions. Hidden commands still execute when invoked by name, so they are useful for engineering-only diagnostics, feature-flagged operations, role-gated commands, etc.

Hiding is always a runtime decision (there is no YAML flag for it), so the decision can depend on environment, license, authenticated role, a CLI flag, or any other context known to the app:

```c++
if( !userIsAdmin ) {
    if( auto cmd = app.commander().find( "debug" ) ) {
        cmd->mHidden = true;
    }
}
```

Commands added programmatically can ship with `mHidden = true` from the start; use this for commands that are never declared in `commands.yml`. See `examples/hidden/` for a full sample, including a `--dev` flag that toggles hiding.

Note: hiding applied after `parse()` only affects `--help` rendering and autocompletion. "Did you mean?" suggestions are emitted during `parse()`. To suppress hidden names from those errors too, set `mHidden` before calling `parse()` using a condition that does not depend on CLI flags.

## Configuration

Use configuration options to modify the behavior of commands and to set values for required parameters.

### Evaluation order

The order of evaluation, from lower to higher priority:

- defaults
- remote config sources (`--config-source`, see below)
- config file
- environment variables
- command-line flags
- explicit call to `Config::set`

### Remote config sources

Drea can load configuration from remote systems using the repeatable `--config-source <uri>` command-line flag. The payload is parsed as JSON and merged into the app's options (nested objects flatten into dotted keys).

Supported schemes:

- `aws://<region>/<secret-id>` — AWS Secrets Manager. Fetches the secret string using the local IAM principal (SDK default credential chain). Requires drea built with `-DENABLE_AWS=ON` and `aws-sdk-cpp[secretsmanager]` installed. An empty region (`aws:///<secret-id>`) falls back to the SDK default region resolution.

Example:

```bash
./myapp --config-source aws://us-east-1/prod/myapp/config
```

The secret is expected to be a JSON document, for instance:

```json
{
  "db": {
    "host": "db.example.com",
    "password": "s3cr3t"
  }
}
```

This populates `db.host` and `db.password` options. The flag is repeatable, so multiple secrets can be merged. See `examples/aws-secrets/` for a runnable sample.

> The legacy `remote-config:` YAML block and `Config::addRemoteProvider` API have been removed. Migrate to `--config-source`.

### Boolean negation

Boolean options can be explicitly disabled from the command line by prefixing them with `--no-`. For example, if `dry-run` defaults to `true` (from a config file or a declared default), passing `--no-dry-run` on the command line overrides it to `false`. This works automatically for any option of type `bool`.

### Sensitive options

Mark an option as sensitive to hide its default value from `--help`. Useful for passwords, API tokens, or keys that are loaded from a config file or a remote source and would otherwise be printed in the help output.

In `commands.yml`:

```yaml
options:
  - option: db-password
    description: Database password
    type: string
    sensitive: true
```

Or programmatically:

```c++
drea::core::Option pw;
pw.mName = "db-password";
pw.mType = typeid( std::string );
pw.mSensitive = true;
app.config().add( pw );
```

The option still parses and loads its value normally; only the help rendering is affected. In `--help`, the default appears as:

```text
--db-password password  Database password. Default (hidden)
```

## Man pages

TODO

## Shell integration

TODO

## Readings

- [On formats](https://news.ycombinator.com/item?id=19653834)
- [Terminology](https://pythonconquerstheuniverse.wordpress.com/2010/07/25/command-line-syntax-some-basic-concepts/): Command-line syntax: some basic concepts
- [Man pages](https://liw.fi/manpages/)

### Meta configs

- [JSonnet](https://jsonnet.org/)
- [Dhall](https://dhall-lang.org/)

HN [discussion]( https://news.ycombinator.com/item?id=19656821 )

### Libs

- [Viper](https://github.com/spf13/viper): Viper is a complete configuration solution for Go applications including 12-Factor apps
- [Cobra](https://github.com/spf13/cobra): Cobra is both a library for creating powerful modern CLI applications as well as a program to generate applications and command files.

- [cpp_cli](https://github.com/TheLandfill/cpp_cli): This repository consists of a library designed to make parsing command line arguments for c++ easy and efficient and a few simple programs showing you how it works.
- [CLI11](https://github.com/CLIUtils/CLI11): CLI11 is a command line parser for C++11 and beyond that provides a rich feature set with a simple and intuitive interface.

- [Consul C++](https://github.com/oliora/ppconsul)

## Quality Checks

### Sonarqube

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=david-antiteum_drea&metric=alert_status)](https://sonarcloud.io/dashboard?id=david-antiteum_drea)

1. Install `build-wrapper` and `sonar-scanner`, and make sure both are on your `PATH`.
2. Set the `SONAR_TOKEN` environment variable.
3. Reconfigure CMake so it picks up `sonar-scanner`. The `sonarqube` target always exists, but it only runs the scanner when `sonar-scanner` was found at configure time; otherwise it fails with a short message explaining what to install.
4. Invoke the target:

```shell
cmake --build build --target sonarqube
```
