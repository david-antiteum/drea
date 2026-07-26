# Configuration

Configuration in Drea is the unified mechanism for everything that is *not* a
positional argument: typed flags, boolean toggles, log destinations, secrets
loaded from a file or a remote source. Drea merges values from several
sources in a fixed order and gives the application a single API to read them.

## Defining options

```yaml
options:
  - option: verbose
    description: increase logging
    short: v
    type: bool

  - option: log-file
    description: log to <file>
    params-names: file
    type: string

  - option: workers
    description: parallel workers
    type: int
    value: 4
```

Fields:

| Field          | Meaning |
|----------------|---------|
| `option`       | Name. Used on the CLI as `--<name>` |
| `short`        | Single-character short form (`-v`) |
| `description`  | Help text |
| `params-names` | Name of the value placeholder in help (`--log-file file`) |
| `params`       | Number of values: `0`, `1`, ..., or `unlimited` |
| `type`         | `bool`, `int`, `double`, `string`. Defaults to `string`, or to `bool` for an option that takes no value |
| `value`        | Default value (single) |
| `values`       | Default values (sequence) |
| `scope`        | `both` (default), `line`, `file`, `none` — where the option is shown in help |
| `sensitive`    | If true, the default is hidden in `--help` |
| `required`     | If true, `parse()` fails when no source provides a value |
| `min` / `max`  | Numeric bounds, validated after source resolution |
| `choices`      | Sequence of legal values; any other value fails validation |
| `negatable`    | `bool` options only, default true. False for an action: `--no-<name>` is refused instead of applied |
| `deprecated`   | If true, flagged as deprecated in `--help` and `describe` |

`bool` options take no value by default — their presence on the CLI flips
them to true. They can be explicitly disabled with `--no-<name>` (see
*Boolean negation* below).

An option that declares neither `type:` nor `params-names:` is a toggle, and
drea infers `type: bool` for it, so these two declarations are the same:

```yaml
- option: round
  description: round the result
- option: round
  description: round the result
  type: bool
```

Without the inference the option would default to `string` while taking no
value: it could never hold anything, `--round` would only mark it as used,
`--no-round` would not apply, and a value set in a config file or the
environment would be ignored. An explicit `type: string` is respected, as is
`scope: none` — the app sets those in code, with a value of its own — and so
is an option declaring `choices`.

## Passing values

Two forms are accepted for any option that takes a value:

```bash
./myapp --log-file /var/log/app.log     # space-separated
./myapp --log-file=/var/log/app.log     # equals form
```

Use the equals form when the value would otherwise be parsed as another
flag — for example a negative number or any string starting with `-`:

```bash
./myapp --threshold=-0.5
./myapp --label=-quiet
```

`--<name>=` (empty right-hand side) sets the option to an empty string for
`string` options. The equals form is rejected on flags that take no value
(`bool` toggles, repeated counters such as `--verbose`).

## Reading options

```cpp
if( app.config().used( "verbose" ) ) {
    app.logger().set_level( spdlog::level::debug );
}

const std::string file = app.config().get<std::string>( "log-file" );
const int workers      = app.config().get<int>( "workers" );

// Repeated options
const auto hosts = app.config().getAll<std::string>( "host" );
```

`used()` returns true when the option was given on the command line, set in
a config file, present in the environment, or has a default value.

`intensity()` returns how many times an option appeared (useful for `-vvv`
verbosity).

### Read toggles with `get<bool>()`, not `used()`

A toggle is a `bool` option — declared, or inferred because it takes no value
(see *Defining options*). `--round` sets it true, `--no-round` sets it false,
and a config file or environment variable carries its value, so the value is
what you must read:

```cpp
if( app.config().get<bool>( "round" ) ) { /* ... */ }
```

`used()` answers a different question — *did any source mention this option* —
so it reads `round: false` in a config file as **on**. Use it for options whose
mere presence is the signal, not for toggles.

## Evaluation order

Sources are applied in this order, lowest priority first. Later values
override earlier ones.

1. Defaults (declared in YAML or via `Option::mValues`).
2. Remote config sources (`--config-source <uri>`, see below).
3. Config file (`--config-file <path>`, or `Config::setDefaultConfigFile`).
4. Environment variables (when an env prefix is set).
5. Command-line flags.
6. Explicit `Config::set( name, value )` calls.

## Config files

`--config-file` is repeatable: files are merged in order and later files
win, the same rule as `--config-source`. Passing any `--config-file` flag
overrides the default file set via `Config::setDefaultConfigFile`.

Drea autodetects the format from the extension when reading
`--config-file <path>`:

- `.toml` — requires the `toml` vcpkg feature (`-DVCPKG_MANIFEST_FEATURES=toml`)
- `.yaml`, `.yml` — always available
- `.json` — available when drea is built with `nlohmann_json` (always pulled in
  by `-DBUILD_REST_USE=ON`; otherwise opportunistic — disabled when the package
  is not found)

Nested objects flatten into dotted keys. Given:

```yaml
db:
  host: db.example.com
  password: s3cr3t
```

values land at `db.host` and `db.password`.

`bool` options read their value from a config file (`round: false` turns the
toggle off, not on). Anything that is not a boolean is a `parse_error`.

## Environment variables

Set an env prefix (in YAML or via `Config::setEnvPrefix`) and Drea will read
options from `<PREFIX>_<NAME>`:

```yaml
app: myapp
env-prefix: MYAPP

options:
  - option: workers
    type: int
```

```bash
MYAPP_workers=8 ./myapp ...
```

Option names may contain characters that are not valid in shell variable
names (for example `config-file` or `db.host`). For those options, Drea also
looks up a variable where every character outside `[A-Za-z0-9_]` is replaced
by `_`:

```bash
MYAPP_config_file=/etc/config.json ./myapp ...   # option: config-file
MYAPP_db_host=localhost ./myapp ...              # option: db.host
```

The conventional all-uppercase spelling is accepted as well:

```bash
MYAPP_CONFIG_FILE=/etc/config.json ./myapp ...
```

Lookup order per option: the exact spelling (`MYAPP_config-file`, for
environments that can set such names, e.g. `env` or `execve`), then the
sanitized spelling (`MYAPP_config_file`), then its uppercase form
(`MYAPP_CONFIG_FILE`). First hit wins.

Only options whose scope permits config sources (`both`, `file`) read the
environment; `line` and `none` scoped options ignore it — a variable set
for one of those is reported (`wrong_scope` in `--validate`) instead of
silently dropped, as is a variable under the prefix that matches no option
(`unknown_key`, non-fatal).

`bool` options read their value from the environment (`true`/`false`,
`yes`/`no`, `1`/`0`):

```bash
MYAPP_dry_run=true ./myapp ...
MYAPP_dry_run=no ./myapp ...
```

Anything else is a `parse_error`, not silently false.

## Remote config sources

Drea can load configuration from remote systems via the repeatable
`--config-source <uri>` flag. The payload is parsed as JSON and merged into
the option set (nested objects flatten into dotted keys).

Supported schemes:

- `aws://<region>/<secret-id>` — AWS Secrets Manager. Uses the SDK default
  credential chain. Requires `-DENABLE_AWS=ON` and `aws-sdk-cpp[secretsmanager]`.
  An empty region (`aws:///<secret-id>`) falls back to the SDK default region.

```bash
./myapp --config-source aws://us-east-1/prod/myapp/config
```

The secret is expected to be a JSON document. Multiple `--config-source`
flags are merged in order (later wins). See
[`examples/aws-secrets/`](../examples/aws-secrets) for a runnable sample.

> The legacy `remote-config:` YAML block and `Config::addRemoteProvider` API
> have been removed. Migrate to `--config-source`.

## Boolean negation

A `bool` option can be turned off explicitly with `--no-<name>`:

```yaml
options:
  - option: dry-run
    type: bool
    value: true     # default ON (e.g. from a config file)
```

```bash
./myapp --no-dry-run    # overrides the default to false
```

### Actions cannot be negated

Some `bool` options are **actions**: they select what the invocation does, and
denying one means nothing. `--help`, `--version` and `--validate` are drea's
own, and they declare `negatable: false`:

```bash
./myapp --no-help       # Option --help is an action and cannot be negated
```

Without that, `--no-help` printed the help, because these are read with
`used()` — presence is their semantics — and the negation registered a use.
The refusal is a `not_negatable` finding, non-fatal like an unknown argument:
the app carries on with the option untouched.

Declare it for an app's own actions, the ones that make the process do
something and quit:

```yaml
options:
  - option: reset-db
    description: recreate the schema and quit
    negatable: false
```

Toggles keep their negation — `--no-verbose`, `--no-log-redact`,
`--no-json` all mean something.

## Sensitive options

Mark an option as sensitive to hide its default value from `--help`. The
option still parses and loads its value normally; only the help rendering
changes.

```yaml
options:
  - option: db-password
    description: database password
    type: string
    sensitive: true
```

In help output:

```
--db-password password  database password. Default (hidden)
```

Useful for secrets loaded from a config file or `--config-source`.

## Validation

Options can declare constraints in the yml definition:

```yaml
options:
  - option: pool-id
    description: cognito pool
    params-names: id
    type: string
    required: true

  - option: port
    description: listen port
    params-names: n
    type: int
    min: 1
    max: 65535
    value: 8080

  - option: color
    description: colorize the output
    params-names: mode
    type: string
    choices: [ auto, always, never ]
    value: auto
```

`App::parse` validates after all sources are resolved (defaults, remote
sources, config file, env, flags): `required` fails when no source provided a
value; `min`/`max` bound each value of numeric options; `choices` restricts
values to a closed set. Every option referenced by a command's
`local-options`/`global-options` must exist, or validation fails too. On
failure every violation is logged at `critical` and the process exits with
`drea::core::ExitCode::ConfigError` (78, sysexits `EX_CONFIG`). `--help` and
`--version` still work when the config is invalid.

`Config::validate()` returns the violation messages without exiting, for apps
that want to handle them differently. `drea::core::ExitCode`
(`include/drea/core/ExitCode.h`) provides the shared exit-code vocabulary
(`Ok`, `ConfigError`, `DependencyError`, ...).

## Checking the configuration: `--validate`

`--validate` is the runtime counterpart of the `describe` command: where
`describe` emits the static command/option tree, `--validate` loads the configuration
from every source (defaults, remote sources, config file, environment,
flags), checks it, reports **all** the problems found — not just the first —
and quits without running any command.

```bash
./myapp --validate                       # human summary on stderr
./myapp --validate --json                # machine readable findings on stdout
./myapp --config-file prod.yml --validate
```

Besides the problems, the report includes the *effective configuration*:
every option that ended up with a value, the value, the source that won,
the declared default when there is one, and whether the setting is
*redundant* — a real source supplying exactly the declared default:

```
myapp: the configuration is valid
Effective configuration:
  port=8080 (from config-file, matches default)
  color=always (from flag)
  log-flush-level=warn (from default)
```

The human summary goes to `stderr`, keeping `stdout` clean for machine
output. With `--json`, the structured report goes to `stdout`:

```json
{
  "schema": "drea-validate/1",
  "app": "myapp",
  "version": "1.2.3",
  "valid": false,
  "findings": [
    {
      "option": "port",
      "source": "config-file",
      "code": "out_of_range",
      "message": "Option --port value 70000 is above the maximum 65535"
    }
  ],
  "effective": [
    { "option": "port", "value": [70000], "default": [8080], "source": "config-file" },
    { "option": "color", "value": ["auto"], "default": ["auto"], "redundant": true, "source": "flag" }
  ]
}
```

Each finding carries the option name (or config key, or dotted command
name), the source that supplied the offending value (`default`,
`config-source`, `config-file`, `environment` or `flag`; omitted when no
single source applies), a human message, and one of these stable codes:

| Code                 | Meaning |
|----------------------|---------|
| `parse_error`        | A value does not parse as the option's declared type, or a config file / remote payload cannot be parsed |
| `file_error`         | The config file cannot be read, or a remote source returned no data |
| `unknown_key`        | A config file key (or a flag) matches no declared option |
| `missing_required`   | A `required` option ended up with no value from any source (a default satisfies it) |
| `bad_choice`         | A value is outside the option's `choices` |
| `out_of_range`       | A numeric value violates `min`/`max` |
| `missing_params`     | A value-taking option was used without satisfying its `params` count |
| `wrong_scope`        | An option was set through a source its `scope` disallows (e.g. a `line` option in a config file) |
| `unknown_option_ref` | A command's `local-options`/`global-options` references an option that does not exist |
| `disabled_group`     | The requested command is gated by groups that are not enabled |
| `bad_source`         | A `--config-source` URI is malformed, uses an unsupported scheme, or needs a feature drea was built without |
| `bad_definition`     | The app declares a constraint that cannot act, e.g. `min`/`max` on a non-numeric option or `choices` on a bool |

Values of `sensitive` options are masked as `[redacted]` in both output
modes.

The output is validatable against the published JSON Schema:
[`docs/validate.schema.json`](validate.schema.json). In validate mode
drea silences its parse-time logging — every problem it would have logged
is a finding — so `stdout` carries nothing but the JSON.

Exit codes map the main failure categories:

| Exit | `ExitCode`    | When |
|------|---------------|------|
| 0    | `Ok`          | The configuration is valid |
| 66   | `NoInput`     | A config file or remote source cannot be read (`file_error`) — beats every other category |
| 78   | `ConfigError` | Structural problems: `unknown_key`, `missing_required`, `wrong_scope`, `unknown_option_ref`, `disabled_group`, `bad_source`, `bad_definition` |
| 65   | `DataError`   | Only values are wrong: `parse_error`, `out_of_range`, `bad_choice`, `missing_params` |

Like `--help` and `describe`, `--validate` works even when the
configuration is invalid — that is its job — and it is registered by
`Config::addDefaults()` (drop it with `Config::remove` if unwanted, `--json`
likewise).

Programmatically, the same model is available as `Config::findings()`
(structured findings) and `Config::declaredDefault(name)` (the default an
option declared before source resolution replaced it), next to
`Config::source(name)` for the winning source. See
[`examples/calculator`](../examples/calculator) for a runnable demo
(`config-invalid.yml`).

## Effective config

With `--log-effective-config`, `App::parse` emits one `info` line per option
with its resolved value and the source that provided it (`default`,
`config-source`, `config-file`, `environment`, `flag` or `code`), flagging
redundant settings — a real source supplying exactly the declared default:

```
config: port=8080 (from config-file, matches default)
config: db-password=[redacted] (from environment)
```

Sensitive options print `[redacted]` (unless `--no-log-redact`).
`Config::source(name)` and `Config::redundant(name)` expose the same
information programmatically. Off by default; services typically turn it on
in their standard options fragment. For a one-shot look at the same facts
without running the app, use `--validate` (see above) — `--log-effective-config`
is its into-the-logs sibling for long-running services.

## Logging

`Config::addDefaults()` registers the logging options and `App::parse` builds
the logger from them:

- `--log-file <file>` / `--log-folder <folder>` — add a rotating file sink
  (`--log-size` MB per file, `--log-nb-files` files kept).
- `--log-flush-level <level>` — flush sinks on messages at or above `<level>`
  (`trace`, `debug`, `info`, `warn`, `err`, `critical`, `off`). Default
  `warn`; `off` disables level-based flushing. Independent of the flush
  level, sinks are flushed every 3 seconds.
- `-v` / `--verbose` — one occurrence enables `debug`, two or more `trace`.
- `--log-redact` (default on) — values wrapped in `drea::log::redacted()`
  print as `[redacted]`. Dev turns it off with `--no-log-redact`. Read once
  at startup and frozen.
- `--log-effective-config` (default off) — dump the effective configuration after
  parsing (see *Effective config* above).

Two header-only helpers under `include/drea/log/` keep client-controlled and
personal data out of production logs:

```cpp
#include <drea/log/Redacted.h>
#include <drea/log/CorrelationId.h>

app.logger().debug( "user email {}", drea::log::redacted( email ) );
const std::string requestId = drea::log::sanitizeCorrelationId( inboundId );
```

`redacted()` wraps any value (strings, numbers) with zero allocation.
`sanitizeCorrelationId()` clamps client-supplied correlation values to
`[0-9A-Za-z._-]`, max 64 chars, and returns empty for anything else — so a
hostile client cannot inject log fields or ANSI escapes.

The console sink prints human-readable text. The file sink writes structured
JSON lines instead, one object per record:

```json
{"timestamp":"2026-07-06T12:34:56.789+02:00","level":"info","logger":"myapp","msg":"listening on :8080"}
```

The message is JSON-escaped, so embedded quotes and newlines do not corrupt
the stream.

Per-call structured fields (`drea::log::Logger`, see the API reference) show
up in both outputs — as extra top-level JSON attributes in the file and as
`[key:value]` blocks on the console:

```
[2026-07-14 10:22:31.045] [myapp] [info] [session:abc-123] License installed
```

```json
{"timestamp":"...","level":"info","logger":"myapp","session":"abc-123","msg":"License installed"}
```

Lines without fields are unchanged. The console pattern is spdlog's default
plus the field blocks; both formatters read the fields on the calling
thread, which is why drea's loggers are — and must stay — synchronous.

The transport is `drea::log::mdc` (`include/drea/log/Mdc.h`), a
thread-local key/value context you can also fill directly for
request-scoped fields — put `session` once and every log line of the
request carries it until you `clear()`. Self-contained: drea does not
depend on `spdlog::mdc` (spdlog ≥ 1.15).

## Option scope

`scope` controls where an option appears in `--help`, and which sources may
set it:

- `both` (default) — top-level options block; read from every source.
- `line` — top-level options block only; command line only, environment
  variables are ignored for it.
- `file` — config-file-only block (separate section in `--help`); read from
  the config sources (remote sources, config file, environment).
- `none` — never shown in help, but still parseable; environment variables
  are ignored for it.

Options declared as `local-options` of a single command are automatically
filtered out of the global help section. They appear only on that command's
per-command help page.

## Defaults from `Config::set`

After `parse()` you can still inject values programmatically:

```cpp
app.config().set( "log-file", "/var/log/myapp.log" );
```

This wins over every other source. Useful for derived defaults that depend on
runtime context.

## Disabling default options

`Config::addDefaults()` registers a built-in option set: `--verbose`,
`--help`, `--version`, `--config-source`, `--log-*`, and (when drea is
compiled with `ENABLE_REST_USE`) `--graylog-host`. An app that does not
support one of these — for example, a service that ships its own logging
backend and does not want users to attempt graylog forwarding — can drop the
option even though drea was compiled with support for it.

Use `Config::remove(name)` to erase any option from the registry. After
removal, `find()` returns null, the option is absent from `--help` and
shell completion, and any matching CLI flag is reported as unknown.

The right hook is `App::configureInRunTime`, which fires after `addDefaults`
and before `Config::configure` reads CLI values:

```cpp
class MyApp : public drea::core::App {
public:
    using drea::core::App::App;

    void configureInRunTime() override
    {
        config().remove( "graylog-host" );    // app does not support graylog
        config().remove( "config-source" );   // app does not load remote config
    }
};

int main( int argc, char * argv[] )
{
    MyApp app( argc, argv );
    app.parse( yaml );
    app.commander().run( /* ... */ );
}
```

`Config::remove` is generic — apps may use it to drop *any* default they do
not need, including options previously declared in their own YAML.
