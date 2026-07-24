# Drea

Drea is a C++17 framework for building command-line tools and services. You
describe the commands and options once in a YAML file, and Drea uses that
description to parse arguments, validate them, render `--help`, generate man
pages and shell completion, and produce a machine-readable description of the
interface.

The goal is a single source of truth. Many option parsers turn `argv` into typed
values and leave the rest to you. Drea also covers the configuration, logging,
and documentation that a longer-lived tool or service usually needs, all derived
from the same declaration, so the help text and the code stay in step.

## Features

- Commands and subcommands, with argument validation.
- Typed, validated options with short forms.
- Configuration from several sources — defaults, remote sources, a config file
  (including TOML), environment variables, and command-line flags — resolved in
  a defined order.
- Optional AWS Secrets Manager as a configuration source, with sensitive values
  masked in help and description output.
- Logging based on spdlog, with an optional Graylog sink.
- `--help` rendering with a dynamic footer hook.
- `--describe`: the full command and option tree (types, bounds, choices,
  defaults, scopes, environment mapping) as versioned JSON, so tools and agents
  can read the interface without parsing `--help`.
- Command groups, for gating help, completion, and execution by tier or role.
- `man` page generation.
- Shell completion for `bash`, `zsh`, and `fish`.

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
- [Help and shell integration](docs/help-and-shell.md) — `--help`, dynamic footer, `man`, completion, `--describe`
- [API reference](docs/api-reference.md) — `App`, `Commander`, `Config`, `Command`, `Option`
- [Examples](docs/examples.md) — index of `examples/`

## Quality

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=david-antiteum_drea&metric=alert_status)](https://sonarcloud.io/dashboard?id=david-antiteum_drea)

To run the SonarQube scan locally, install `build-wrapper` and `sonar-scanner`,
set `SONAR_TOKEN`, reconfigure CMake (so it picks up `sonar-scanner`), and:

```bash
cmake --build build --target sonarqube
```

## Inspiration and alternatives

Drea follows the design that Go's CLI ecosystem settled on:
[Cobra](https://github.com/spf13/cobra) for the command/subcommand structure
with generated help, completion, and man pages, and
[Viper](https://github.com/spf13/viper) for layered configuration (defaults,
config file, environment, flags, and remote sources). Drea combines both around
a single declarative spec in C++17.

C++ already has several good argument parsers. Most of them focus on parsing
rather than on the wider app/service setup, so they make different trade-offs.
A rough comparison:

| Library | Stars | Status | Notes |
| --- | --- | --- | --- |
| [CLI11](https://github.com/CLIUtils/CLI11) | ~4.4k | Active | Header-only, no dependencies, C++11. Strong subcommand support and config-file (TOML/INI) plus environment variables. Options are set up imperatively in C++ rather than declared; no remote/secret source, logging, or machine-readable description. |
| [cxxopts](https://github.com/jarro2783/cxxopts) | ~4.8k | Active | Header-only C++11, small and quick to adopt, Boost.Program_options-style syntax. No real subcommands, config files, or environment sources; no man/completion generation. |
| [p-ranav/argparse](https://github.com/p-ranav/argparse) | ~3.5k | Active | Single-header C++17 with an API close to Python's argparse, including subparsers. No config-file, environment, or remote sources; no man/completion generation. |
| [gflags](https://github.com/gflags/gflags) | ~3.0k | Maintenance only | Long used at scale; flags can be defined across translation units. No longer developed by Google (Abseil Flags is the successor); no subcommands or layered configuration. |
| [docopt.cpp](https://github.com/docopt/docopt.cpp) | ~1.1k | Last release 2020 | Fully declarative — the parser is generated from the help text, which is the closest approach to Drea's. Effectively unmaintained; no configuration sources and limited validation. |
| [Boost.Program_options](https://github.com/boostorg/program_options) | (part of Boost) | Mature | Merges config file, environment, and command line; a natural fit where Boost is already a dependency. Verbose pre-C++11 API, no subcommands, and no man/completion generation. |

A note on where Drea costs more: unlike CLI11, cxxopts, and argparse, it is not
header-only. It builds with CMake and vcpkg and brings in dependencies (spdlog,
a YAML parser, and optionally the AWS SDK), it requires C++17, and it is younger
and less widely used than the parsers above. For a single binary that only needs
to read a few flags, a header-only parser is the simpler choice. Drea fits when
the declarative spec, layered configuration, and service features are useful
together.

## Further reading

- [Command Line Interface Guidelines (clig.dev)](https://clig.dev/) — modern,
  opinionated conventions for CLI design
- [POSIX Utility Conventions](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html)
  — the base standard for utility syntax: options, option-arguments, operands,
  and the `--` separator
- [GNU Program Argument Syntax Conventions](https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html)
  — long options and the widely-followed GNU extensions
- [Man pages](https://liw.fi/manpages/)
- [On formats](https://news.ycombinator.com/item?id=19653834)
- [AGENTS.md](https://agents.md/) — a convention for guiding AI agents, related
  to Drea's `--describe`
