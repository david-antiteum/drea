# Changelog

All notable changes to drea are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
