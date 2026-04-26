# Drea

A C++17 framework for CLI apps and services. Declare commands and options once
in YAML; Drea derives the rest.

## Features

- Commands and subcommands (with argument validation)
- Options (typed, validated, with short forms)
- Configuration from multiple sources (file, env, flags, AWS Secrets Manager)
- Logging (spdlog-based, with optional Graylog sink)
- `--help` rendering with a dynamic footer hook
- Command groups for tier/role-based gating of help, completion, and execution
- `man` page generation
- Shell completion (`bash`, `zsh`, `fish`)

## A 30-second example

`commands.yml`:

```yaml
app: say
version: 0.0.1
description: Prints the argument of the command "this" and quits.
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

`main.cpp`:

```c++
#include <drea/core/Core>
#include <algorithm>
#include "commands.yml.h"

int main( int argc, char * argv[] )
{
    drea::core::App app( argc, argv );

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

```bash
$ ./say this hello
[2019-04-16 09:44:44.649] [say] [info] hello
$ ./say this hello --reverse
[2019-04-16 09:44:44.690] [say] [info] olleh
```

## Build

Drea ships CMake presets (CMake >= 3.21 + Ninja). Point `VCPKG_ROOT` at your
vcpkg checkout, then:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Available presets: `debug`, `release`, `sdk`, `debug-system`, `release-system`,
`sdk-system`. The `*-system` variants use system-installed dependencies
instead of vcpkg.

Opt-in vcpkg features:

```bash
cmake --preset debug -DVCPKG_MANIFEST_FEATURES=toml        # TOML config files
cmake --preset debug -DVCPKG_MANIFEST_FEATURES=aws         # AWS Secrets Manager
cmake --preset debug -DVCPKG_MANIFEST_FEATURES="toml;aws"
```

To enable AWS Secrets Manager at runtime, pass `-DENABLE_AWS=ON`.

Bare configure (no preset) also works:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Use Drea in your project

```cmake
find_package(drea REQUIRED)
target_link_libraries(main PRIVATE drea)
```

## Documentation

- [Commands](docs/commands.md) — anatomy, parameters, hierarchy, hiding, groups
- [Configuration](docs/configuration.md) — options, sources, evaluation order, sensitive values
- [Help and shell integration](docs/help-and-shell.md) — `--help`, dynamic footer, `man`, completion
- [API reference](docs/api-reference.md) — `App`, `Commander`, `Config`, `Command`, `Option`
- [Examples](docs/examples.md) — index of `examples/`

## Quality

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=david-antiteum_drea&metric=alert_status)](https://sonarcloud.io/dashboard?id=david-antiteum_drea)

To run the SonarQube scan locally, install `build-wrapper` and `sonar-scanner`,
set `SONAR_TOKEN`, reconfigure CMake (so it picks up `sonar-scanner`), and:

```bash
cmake --build build --target sonarqube
```

## Further reading

- [Command-line syntax: some basic concepts](https://pythonconquerstheuniverse.wordpress.com/2010/07/25/command-line-syntax-some-basic-concepts/)
- [Man pages](https://liw.fi/manpages/)
- [On formats](https://news.ycombinator.com/item?id=19653834)

Similar libraries: [Cobra](https://github.com/spf13/cobra) (Go),
[Viper](https://github.com/spf13/viper) (Go),
[CLI11](https://github.com/CLIUtils/CLI11),
[cpp_cli](https://github.com/TheLandfill/cpp_cli).
