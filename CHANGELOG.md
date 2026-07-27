# Changelog

All notable changes to drea are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Root params**: an app can declare the positional arguments it takes with
  no command at all, through the top level `root:` block in the YAML
  (`params-names`, `params`, `min-params`, `param-choices`, `description`,
  `examples`) or `Commander::setRoot()`. The arguments arrive through
  `arguments()` with an empty command name, and are counted and checked
  exactly like the params of a command. `Commander::root()` exposes the
  declaration; `--help`, `man` and `describe` (new `root` object, `usage`
  now lists every accepted form) render it. A command still wins over root
  params: a leading `--` forces the remaining arguments through
  (`./hello -- man`).
- `Commander::hasAppCommands()` — commands of the app itself, ignoring the
  builtins — and `Commander::invalidCommand()`, true when the arguments could
  not be dispatched.

### Changed

- An argument that is not a command, in an app that declares no root params,
  is a usage error: drea reports it (as before) and now quits with
  `ExitCode::UsageError` (64) **without** calling the `run()` callback. It
  used to log `Unknown command "…"` and then dispatch an empty command, so a
  mistyped invocation could still do real work. `--help` and `--version` keep
  working on a typo.
- The `usage:` line follows what the app declares instead of always demanding
  a `COMMAND`: apps with only the builtins get `[COMMAND]`, apps with root
  params get their own form (both lines when they have commands too). Only
  commands of the app itself make `COMMAND` required.
- Positional params in help and man render `<name>` when required and
  `[name]` when optional, with a trailing `...` for `params: unlimited`.
  Required params used to be printed as `[name]`, reading as optional.
- The `man` and `completion` builtins document what to do with their output:
  the description says where it goes and the examples show loading and
  installing the completion script and reading or saving the man page.
- Multi line descriptions stay one line where a single line is required: the
  command lists in `--help`, the zsh/fish completion scripts and the man
  `NAME` section use the first line (`utilities::string::firstLine`). The man
  `NAME` section of an app with a multi line description was malformed.
- `--` on the command line ends option parsing: it is no longer passed to the
  command as an argument.

- An option declared in YAML with neither `type:` nor `params-names:` is a
  toggle, and its type is inferred as `bool`. It used to default to `string`
  while taking no value, so it could never hold anything: `--opt` only marked
  it as used, `--no-opt` did not apply, and a value from a config file or the
  environment was ignored — the trap the `calculator` sample fell into. An
  explicit `type: string` is respected, and `scope: none` (the app sets those
  in code) and options declaring `choices` are exempt. Options built
  programmatically are untouched: `mType` is right there in the struct.

### Removed

- **`--describe` is gone**: the app description is printed by the `describe`
  builtin **command** (`myapp describe`). No alias — `--describe` is now an
  unknown option. Rationale: `man`, `completion` and `describe` emit a
  standalone artifact about the app and ignore the rest of the command line, so
  they are commands; `--help`, `--version` and `--validate` answer about *this*
  invocation (`myapp deploy --help`, `--validate` on the args actually passed),
  so they stay flags. `describe` still works when the configuration is invalid,
  and an app defining its own `describe` command keeps it, as with the other
  builtins. `Commander::remove("describe")` drops it;
  `Config::remove("describe")` no longer applies.

### Fixed

- The man page never rendered `examples`, although the docs promised it was
  built from the same metadata as `--help`: a command's worked invocations
  reached `--help` and `describe` but not `man`. Both page builders now emit an
  `EXAMPLES` section (unfilled, hyphens escaped so a flag stays copy
  pasteable, a leading `.` guarded so roff does not read the line as a
  request), and the app page carries the root `description` and `examples` as
  well. The synopsis of a command page no longer ends in a trailing space,
  which `mandoc -Tlint` reported.

- `params: unlimited` on an *option* consumed nothing. `numberOfParams()`
  returns the `mUnlimitedParams` sentinel, which is negative as an `int`, so
  the value-consuming loops (`np < numberOfParams()`) never ran and the inline
  form `--opt=value` was rejected as "does not take a value". The option now
  consumes arguments until the next option, a `--` terminator or the end of the
  command line, and accepts the inline form. New `Option::unlimitedParams()`
  and `Option::takesValues()` express the test the sentinel breaks; use those
  rather than comparing `numberOfParams()`.

- `values:` defaults were always stored as strings, ignoring the declared
  `type:`, so an `int` option with `values: [80, 443]` threw
  `std::bad_variant_access` through `Config::get<int>()` or
  `Option::toString()`. Each entry now goes through `Option::fromString`, the
  same conversion every other source uses, and a default that does not fit the
  type is a fatal declaration error. Conversion happens once the whole option
  is parsed, so `values:` may appear before `type:`. Without a `type:` the
  values stay strings, which is what the default `string` type means.
  the `describe` builtin reports typed defaults accordingly (`[80, 443]`, not
  `["80", "443"]`).

- A config file could trigger an action or set any command-line-only option:
  the YAML, JSON and TOML readers never checked `scope`, so `help: true` in a
  config file printed the help on every run. All three now go through
  `Config::acceptsCurrentSource()` — the same gate the environment scan uses,
  and where the scope rules now live — which reports `wrong_scope` and leaves
  the option untouched instead of applying the value. `help`, `version` and
  `validate` are `Scope::Line` accordingly. The finding is a warning during a
  normal run and exit 78 under `--validate`, as before.

- `--no-help` printed the help, `--no-version` printed the version and
  `--no-validate` ran the validation (reporting `validate=false (from flag)`
  while doing it). Negation was offered for every `bool` option, and these are
  read with `used()`, so denying one registered a use and triggered the very
  action being denied. `Option::mNegatable` (yml `negatable: false`) marks an
  action: `--no-<name>` is then refused with a `not_negatable` finding —
  non-fatal, like an unknown argument — instead of being applied. Set for
  `help`, `version` and `validate`; toggles such as `--no-verbose`,
  `--no-log-redact` and `--no-json` are unaffected. `describe` exposes
  `"negatable": false` for the options that carry it.

- Config files ignored the value of a `bool` option: the YAML, JSON and TOML
  readers registered the option as used and parsed a value only when it had a
  param name, so `round: false` left the toggle on. All three now parse the
  scalar the way the environment path already did, so a config file can turn a
  toggle off, and a bad value (`round: banana`) is a `parse_error` finding.
  The `calculator` sample carried the matching app-side bug: its `round`
  option declared no `type:` — making it a valueless `string` option, readable
  only through `used()` — and it now declares `type: bool` and is read with
  `get<bool>()`.
## [0.37.1] — 2026-07-25

### Fixed

- Linux build: the `environ` declaration used for the unknown-env-var scan
  now has C linkage, matching glibc's declaration in `unistd.h` (pulled in
  transitively by boost). 0.37.0 did not compile on Linux.

## [0.37.0] — 2026-07-24

### Added

- `--validate` reports the *effective configuration* next to the findings:
  every option that ended up with a value, its source, the declared default,
  and a redundant marker when a real source supplied exactly the default.
  Human mode prints `port=8080 (from config-file, matches default)`; JSON
  gains a required `effective` array. `Config::redundant(name)` exposes the
  check programmatically.

- `--validate` (with `--json` for machine output): load the configuration
  from every source, check it, report **all** the problems and quit without
  running any command. Findings carry the option, the offending source, a
  human message and a stable code (`parse_error`, `file_error`,
  `unknown_key`, `missing_required`, `bad_choice`, `out_of_range`,
  `missing_params`, `wrong_scope`, `unknown_option_ref`, `disabled_group`);
  sensitive values are masked. Exit codes: 0 valid, 66 unreadable config
  file, 78 structural problems, 65 bad values. Human output goes to stderr,
  `--json` (`drea-validate/1`, JSON Schema in `docs/validate.schema.json`)
  to stdout; parse-time logging is silenced in validate mode so the report
  is the only output. Both options are registered by `Config::addDefaults`.
- `Config::findings()` — the structured resolved-config checks behind
  `--validate` — and `Config::declaredDefault(name)`, the default an option
  declared before source resolution replaced it. `Option::typeName()` and
  `Option::scopeName()` expose the closed sets used by `--describe`.
- `param-choices` on commands: a closed set of legal values for the
  positional param of a command taking exactly one (`params: 1`). Checked
  at dispatch, rendered by `--help`, `man` and `--describe`, offered by
  shell completion. The `completion` builtin declares `bash`/`zsh`/`fish`
  this way. Declaring it on a command with a different param count is a
  `bad_definition` finding.

### Fixed

- `--help` and `--describe` showed the *resolved* value as `Default` — e.g.
  `myapp --tier operator --help` claimed "Default operator" for an option
  with no default. Both now report the declared default (snapshot before
  source resolution), and `--help` additionally prints
  `Current value <v>` when a flag, env var or config source set the option
  (`(hidden)` for sensitive options). `--describe` stays static: declared
  default only.

- `--config-source` problems are findings now: an unsupported scheme, a
  malformed `aws://` URI or a build without `ENABLE_AWS` report
  `bad_source`; a source that returns no data reports `file_error`; an
  unparseable payload reports `parse_error`. Previously all of these were
  log lines only and validation said "valid".
- `bad_definition` findings for declared constraints that cannot act:
  `min`/`max` on a non-numeric option, `choices` on a bool. Like
  `bad_source`, fatal in `App::parse` (`ExitCode::ConfigError`).
- The environment is checked like the other sources: a variable set for an
  option whose scope does not read the environment is reported
  (`wrong_scope`), and a variable under the prefix that matches no option
  in any accepted spelling is reported (`unknown_key`). Both non-fatal.
- Shell completion (bash, zsh, fish) offers the values of options that
  declare `choices`, and man pages render the computed facts help already
  shows: `One of: ...`, the declared default, `Required`, deprecation.

### Changed

- Structured log fields ride on `drea::log::mdc`
  (`include/drea/log/Mdc.h`), a self-contained thread-local context owned
  by the drea library, instead of `spdlog::mdc` — drea no longer requires
  spdlog ≥ 1.15. **Breaking** for code that put request-scoped context
  into `spdlog::mdc` directly: swap the namespace, same put/get/remove/
  clear/get_context API. As before, only synchronous loggers see the
  context.
- `--config-file` is repeatable: files are merged in order, later wins
  (same rule as `--config-source`). Previously only the first flag was
  read and the rest were silently ignored.
- `bool` options now read their value from the environment
  (`MYAPP_dry_run=false` works); before, the variable's presence only
  marked the option as used and `get<bool>` stayed false.
- **Breaking:** `Option::fromString` is strict about scalar syntax: bool
  values must be `true`/`false`, `yes`/`no` or `1`/`0` (anything else was
  silently `false` before), and numbers reject trailing characters
  (`"80x"` parsed as `80` before). Bad values follow the normal
  `parse_error` path.
- A value that does not parse as its option's declared type is now collected
  and reported together with every other validation error (`App::parse`
  exits with `ExitCode::ConfigError`), instead of `exit(-1)` at read time.
  `Config::set` after parsing keeps the old report-and-exit contract. A
  config file that cannot be parsed is fatal in `App::parse` too; an
  unreadable one stays a logged error (and a `--validate` finding).
- `.yml` config files are read directly as YAML instead of going through
  format autodetection.
- **Breaking:** the `--log-config` predefined option is renamed to
  `--log-effective-config` — it dumps the effective configuration, and the
  old name read as "configure the log" next to `--log-file`/`--log-size`.
  It now flags redundant settings too (`..., matches default`). Env var:
  `<PREFIX>_log_effective_config`.
- `log-flush-level` declares `choices` (`trace`, `debug`, `info`, `warn`,
  `err`, `critical`, `off`): an unknown level now fails validation
  (`bad_choice`, `ExitCode::ConfigError`) instead of silently falling back
  to `warn`. The spdlog alternate spellings `warning`/`error`, which
  happened to parse before, are rejected — use `warn`/`err` as documented.
  Help renders the set from the same metadata.

## [0.36.0] — 2026-07-14

### Added

- `drea::log::Logger` gains call-site passthroughs: `log( level, ... )` with
  the same plain/one-field/multi-field overloads as the named levels (so a
  runtime-chosen severity keeps its structured fields), `should_log( level )`
  to guard expensive argument computation, and `flush()`.

### Changed

- **Breaking:** `App::logger()` returns `drea::log::Logger &` (the
  structured-fields wrapper) instead of `spdlog::logger &`, so
  `app.logger().debug( { "session", id }, "..." )` works directly.
  `Config::logEffective` takes the wrapper too. Plain level calls
  (`logger().info( "..." )` etc.) and `flush()` compile unchanged; other
  spdlog-specific calls (levels, sinks) move to `logger().raw()`. The
  returned wrapper
  always targets the current logger: the spdlog default one before
  `App::parse`, the configured one after.

## [0.35.0] — 2026-07-14

### Added

- Per-call structured log fields: `drea::log::Logger` + `drea::log::Field`
  (`include/drea/log/Logger.h`). Wrap the spdlog logger once and pass fields
  per call — `logger.info( { "session", id }, "..." )`. Fields are emitted
  as top-level attributes in the JSON file log and as `[key:value]` blocks
  on the console; lines without fields are unchanged. Values compose with
  `drea::log::redacted()`. Transported through `spdlog::mdc`, so entries put
  there directly (request-scoped context) appear in both outputs too.

### Changed

- `Config::setupLogger` sets an explicit console pattern (spdlog's default
  plus the structured-field blocks). Loggers must stay synchronous: an async
  logger would format on a backend thread and silently drop all fields.

## [0.34.0] — 2026-07-06

### Added

- Declarative option validation: `required: true`, `min:` and `max:` keys in
  the yml definitions. `App::parse` validates after source resolution, logs
  every violation at `critical` and exits with `ExitCode::ConfigError` (78).
  `--help`/`--version` still work when the config is invalid.
  `Config::validate()` returns the messages without exiting.
- `drea::core::ExitCode` (`include/drea/core/ExitCode.h`): sysexits-inspired
  exit-code vocabulary (`Ok`, `UsageError`, `DependencyError`, `ConfigError`,
  ...) shared by CLIs and services. Fatal yml-definition errors in `parse`
  now exit with `ConfigError` instead of 1.
- `drea::log::redacted()` (`include/drea/log/Redacted.h`): wrap values that
  must not reach production logs; prints `[redacted]` unless the new
  `log-redact` predefined option (default on) is disabled with
  `--no-log-redact`. Zero allocation, works with any fmt-formattable value.
- `drea::log::sanitizeCorrelationId()` (`include/drea/log/CorrelationId.h`):
  clamp client-supplied correlation values (request ids, session ids) to
  `[0-9A-Za-z._-]`, max 64 chars, else empty. Header-only, pure std.
- Effective-config logging: new `--log-config` predefined option (default
  off) makes `App::parse` emit one info line per set option with its
  resolved value and source; sensitive values redacted. `Config::source()`
  and `Config::logEffective()` expose the same data programmatically.
- Per-option source tracking across the resolution order: `default`,
  `config-source`, `config-file`, `environment`, `flag`, `code`.

### Changed

- `--help` now separates application items from drea's built-ins: options
  from `Config::addDefaults()` are listed under *Common options* and the
  `completion`/`man` commands under *Common commands*, after the app's own
  *Options*/*Commands* sections. New `mPredefined` flag on `Option` and
  `Command` drives the split.

## [0.33.0] — 2026-07-06

### Added

- `--log-flush-level <level>` predefined option (default `warn`): flush log
  sinks on messages at or above that level. `off` disables level-based
  flushing; unknown values fall back to `warn` with a warning.
- The rotating file sink (`--log-file` / `--log-folder`) now writes JSON
  lines (`timestamp` ISO 8601 with offset, `level`, `logger`, `msg`, with
  proper JSON escaping) while the console keeps the human-readable text
  format.

### Fixed

- Loggers built by `Config::setupLogger()` had no flush policy: with
  `--log-folder` the log file stayed empty until process exit. Now they
  flush on `warn` (configurable) and every 3 seconds.

## [0.32.0] — 2026-04-26

### Added

- `--opt=value` syntax on the command line. Works for any option that takes
  a value and is the way to pass values that begin with `-` (negative
  numbers, hyphen-leading strings) since the space-separated form treats a
  leading `-` as the start of the next flag.
- Tests covering `--opt=value` for `string`/`int`/`double`, hyphen-leading
  values, empty right-hand side, and bool enable/`Config::set` paths.

### Changed

- A bool option declared with `type: bool` (no params) now has its value
  set to `true` when its `--flag` is present on the CLI, so
  `config().get<bool>(name)` returns `true`. Previously `--flag` only
  registered the option as used and `get<bool>` returned `false`.
- `Config::set( name, value )` now also calls `registerUse( name )`, so
  `used()` returns `true` after a programmatic override — matching the
  documented contract.
- Numeric positional arguments that begin with `-` (`-1`, `-0.5`,
  `-.25`) are no longer eaten by the short-option expander. Parser
  treats `-` followed by a digit or `.` as a literal positional, so
  `mycli sum -1 2 3` reaches the command unchanged.

### Fixed

- `docs/api-reference.md` description of `App::configureInRunTime()` now
  reflects the actual call site (after `addDefaults`, before
  `Config::configure`), not "at the start of `parse()`".
- `docs/configuration.md` no longer claims `.json` config-file support is
  "always available"; it depends on `nlohmann_json` being present at build
  time when `BUILD_REST_USE=OFF`.

## [0.31.0] — 2026-04-26

### Added

- `Command::mGroups` — declarative visibility gating. A command without
  groups is always visible; a command with groups is visible only when at
  least one of its groups has been enabled at runtime.
  - YAML key `group:` accepts either a scalar (`group: staff`) or a sequence
    (`group: [staff, beta]`).
  - Subcommands inherit their parent's groups when they declare none of
    their own; explicit groups on a child override the parent.
- `Commander::setEnabledGroups`, `Commander::enabledGroups`,
  `Commander::isVisible(cmd)` — runtime API for the group system.
- `Commander::requestedCommand()` — dotted path of the command parsed from
  argv (empty when no command was given).
- `App::setHelpFooter(fn)` and `App::helpFooter(cmd)` — install a callback
  that returns dynamic text appended to `--help`. Receives the dotted
  command path (or empty for top-level help) so the footer can vary by
  context.
- `Config::remove(name)` — drop an option from the registry, including
  defaults added by `Config::addDefaults` (e.g. `graylog-host`,
  `config-source`). Useful for apps that do not want to expose features
  drea was compiled with.
- `Commander::remove(cmdName)` — drop a command (and its descendants) from
  the registry by dotted name. Useful for apps that do not want the `man`
  or `completion` builtin, or that need to retract a programmatically added
  command.
- `examples/groups/` — runnable sample demonstrating tier-style gating and
  the dynamic footer.
- `docs/` — split user manual: `commands.md`, `configuration.md`,
  `help-and-shell.md`, `api-reference.md`, `examples.md`. README slimmed to
  a landing page.

### Changed

- The visibility filter now runs inside `Commander::run()` and applies
  uniformly to `--help`, `bash`/`zsh`/`fish` completion, `man`,
  "did you mean?" suggestions, and direct invocation. Previously `mHidden`
  was honoured by help/completion but not by execution or by the
  unknown-command suggestion path.
- Invoking a hidden or gated command now emits the same
  `Unknown command "X". Did you mean "Y"?` error as a typo and skips the
  callback. A probe and a mistype are byte-identical on stderr.
- Per-command help on a non-visible command no longer renders the page;
  the unknown-command error is emitted instead, so descriptions of gated
  commands cannot leak via `myapp gated --help`.
- Unknown-command output now reports the command in space-separated form
  (`"cost top"` rather than `"cost.top"`) for symmetry with the existing
  "requires a sub command" branch.

### Fixed

- `Commander::unknownCommand` previously misclassified hidden leaves as
  "requires a sub command". They are now reported as unknown with a
  Levenshtein suggestion.
- The `man` per-command renderer now treats invisible targets the same as
  unknown ones and falls back to the top-level page rather than emitting
  a page for a gated command.

### Removed

- Nothing.

## [0.30.1] — 2026-04-20

Last release before the changelog was introduced. Highlights from the
preceding tag history:

- `b1c1d7a` Add man page and shell completion builtins for Unix.
- `7276c2c` Fix `/thirdparty` path in installed `dreaTargets`.
- `f3e7104` Modernize CMake: `target_*`, proper package config,
  feature-gated vcpkg deps.
- `42c1868` Add `--config-source`, runtime-hidden commands, sensitive
  options, and CMake presets.
- `081c258` Add validation, optional args, negation, local-option
  filtering, and tests.
