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
| `type`         | `bool`, `int`, `double`, `string` |
| `value`        | Default value (single) |
| `values`       | Default values (sequence) |
| `scope`        | `both` (default), `line`, `file`, `none` — where the option is shown in help |
| `sensitive`    | If true, the default is hidden in `--help` |

`bool` options take no value by default — their presence on the CLI flips
them to true. They can be explicitly disabled with `--no-<name>` (see
*Boolean negation* below).

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

Drea autodetects the format from the extension when reading
`--config-file <path>`:

- `.toml` — requires the `toml` vcpkg feature (`-DVCPKG_MANIFEST_FEATURES=toml`)
- `.yaml`, `.yml` — always available
- `.json` — always available

Nested objects flatten into dotted keys. Given:

```yaml
db:
  host: db.example.com
  password: s3cr3t
```

values land at `db.host` and `db.password`.

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

## Option scope

`scope` controls where an option appears in `--help`:

- `both` (default) — top-level options block.
- `line` — top-level options block only.
- `file` — config-file-only block (separate section in `--help`).
- `none` — never shown in help, but still parseable.

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
