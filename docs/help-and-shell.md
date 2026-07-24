# Help and shell integration

Drea generates `--help`, a `man` page, and shell completion scripts from the
same metadata you used to declare commands and options. Hidden and gated
commands are filtered out of every integration uniformly.

## `--help`

`./myapp --help` produces a description, a `usage:` line, an *Options*
section, an optional *Config file options* section, and a *Commands*
section.

Application items are separated from drea's built-ins: options registered by
`Config::addDefaults()` (`--log-*`, `--config-*`, `-v`, `-h`, `-V`, ...) are
listed under *Common options* after the app's own *Options*, and the
commands added by `Commander::addDefaults()` (`completion`, `man`) under
*Common commands* after the app's *Commands*. A section that would be empty
is omitted. The split is driven by the `mPredefined` flag on `Option` and
`Command`, which both `addDefaults()` set — an app can flip it to move an
item between sections.

`./myapp <cmd> --help` (or `./myapp <cmd> <subcmd> --help`, etc.) produces a
per-command help page with the command's own description, its subcommands,
its local options, and its global options.

Filtering rules:

- Commands hidden via `mHidden` are omitted.
- Commands gated by groups not currently enabled are omitted (see
  [Commands → Command groups](commands.md#command-groups)).
- Options exclusively used as `local-options` of a single command are
  omitted from the top-level help and shown only on that command's page.
- Options with `scope: none` are never shown.
- Options with `scope: file` move to the *Config file options* section.

Each option line ends with its computed facts: `One of: ...` (declared
`choices`), `Default ...` (the *declared* default), and `Current value ...`
when a real source — flag, environment, config file, remote source — set the
option; a value that merely comes from the default shows `Default` alone.
For `sensitive` options both render as `(hidden)`:

```
--log-size size    log <size> (in MB) for each log file. Default 10. Current value 20
--tier tier        simulated staff tier. One of: none, readonly, operator. Current value operator
```

## `--describe`

`./myapp --describe` prints the full app description — commands (with
sub commands), options and their limits — as a single JSON object on
stdout and quits. It is aimed at AI agents and other tools that want to
discover the CLI without scraping `--help`.

```json
{
  "schema": "drea-describe/1",
  "app": "myapp",
  "version": "1.2.3",
  "description": "...",
  "usage": "myapp COMMAND [SUBCOMMAND ...] [PARAMS] [OPTIONS]",
  "conventions": {
    "option-syntax": "pass options as --name value or --name=value; an option with a short version also accepts -x",
    "bool-options": "bool options are flags: --name enables, --no-name disables",
    "option-types": ["bool", "int", "double", "string"],
    "option-scopes": ["both", "command-line", "config-file", "none"],
    "option-fields": "scope tells where an option may be set; min and max bound numeric values; choices is the closed set of legal values; nb-params is the fixed number of values the option takes per use (commands instead declare a min-params/max-params range for their positional params)",
    "values": "every option value is a list: default is always an array, and any value-taking option may be repeated with values accumulating, so a scalar-looking option passed twice carries two elements. The app reads the first element when it expects a single value (first wins). Repeating a flag increases its intensity (-vv). The default of a sensitive option is masked as the string [redacted]",
    "required-options": "required means the option must end up with a value from any source; a default already satisfies it",
    "scopes": "both = command line plus config sources; command-line = flags only; config-file = config sources only, which bundle remote sources, the config file and environment variables; none = not set by users, the app sets it in code (listed so its meaning is known). An option reads the environment only when its scope permits config sources AND it carries an env field",
    "env-derivation": "an option is read from the variable env-prefix + '_' + the option name with every character outside [A-Za-z0-9_] replaced by '_'; the all-uppercase spelling is also accepted. The env field gives the exact name per option",
    "command-options": "a command accepts its local-options and global-options; global-options are also accepted by its subcommands",
    "command-groups": "a command listing groups is only available when one of those groups is enabled by the app; commands gated by disabled groups are omitted from this description",
    "config-precedence": "defaults, then remote config sources, then the config file, then environment variables, then command line flags; later sources win"
  },
  "env-prefix": "MYAPP",
  "options": [
    {
      "name": "threshold",
      "description": "detection threshold",
      "type": "double",
      "param-name": "value",
      "nb-params": 1,
      "min": 0,
      "max": 1,
      "default": [0.5],
      "env": "MYAPP_threshold",
      "scope": "both",
      "required": false,
      "sensitive": false
    },
    {
      "name": "color",
      "description": "colorize the output",
      "type": "string",
      "param-name": "mode",
      "nb-params": 1,
      "choices": ["auto", "always", "never"],
      "default": ["auto"],
      "env": "MYAPP_color",
      "scope": "both",
      "required": false,
      "sensitive": false
    }
  ],
  "commands": [
    {
      "name": "repeat",
      "description": "repeat something",
      "min-params": 0,
      "max-params": 0,
      "local-options": ["reverse"],
      "global-options": [],
      "commands": [
        {
          "name": "parrot",
          "description": "print parrot",
          "min-params": 0,
          "max-params": 0,
          "local-options": ["reverse"],
          "global-options": [],
          "commands": [
            { "name": "blue", "...": "..." },
            { "name": "red", "...": "..." }
          ]
        }
      ]
    }
  ]
}
```

Notes:

- Commands form a tree that mirrors the command line: a command's
  subcommands are nested objects under its `commands` key (omitted for
  leaf commands). The words of an invocation are the `name`s along the
  path — the example above describes `myapp repeat parrot blue`.
- `nb-params` / `max-params` are numbers, or the string `"unlimited"`.
- `min` / `max` appear only when the option declares limits; `default`
  only when the option has a default. Defaults of sensitive options are
  emitted as the string `"[redacted]"`.
- `env` gives the exact environment variable name (prefix applied,
  invalid characters mapped to `_`). It is present only when an env
  prefix is set AND the option's scope permits config sources — "does
  this option read from env?" is exactly "does it carry an `env` field?".
- `deprecated: true` appears only on deprecated options/commands;
  `examples` only on commands that declare worked invocations. Absent
  means not deprecated / no examples.
- `scope` is one of `"both"`, `"command-line"`, `"config-file"` or
  `"none"`; the closed sets for `scope` and `type` are also emitted
  machine-readable as `conventions.option-scopes` and
  `conventions.option-types`.
- A command gated by groups carries a `groups` array when visible; gated
  commands whose groups are disabled are omitted entirely.
- The same filtering as `--help` applies to commands: hidden and gated
  commands are omitted. All registered options are listed, with their
  `scope` field telling where each one may be used.
- Like `--help`, it works even when the current configuration is invalid.
- The output is validatable against the published JSON Schema:
  [`docs/describe.schema.json`](describe.schema.json). Every name in a
  command's `local-options`/`global-options` refers to an entry in the
  top-level `options` array — `App::parse` enforces this invariant.

`--describe` has a runtime counterpart: `--validate` checks the
configuration resolved from every source and reports the problems (with
`--json` for machine output). See
[Configuration → Checking the configuration](configuration.md#checking-the-configuration---validate).

## Dynamic help footer

Install a callback to append text to `--help`. The callback is invoked once
per help render; the second argument is the dotted command path
(`"git.commit"`) or empty for top-level help. Return an empty string to
skip the footer for that call.

```cpp
const bool anonymous = !hasCachedSession();
app.setHelpFooter( [anonymous]( const drea::core::App &, std::string_view command ){
    if( anonymous && command.empty() ){
        return std::string( "More commands available after \"myapp auth login\"." );
    }
    return std::string{};
});
```

The text is printed verbatim at the end of the help output, with one
leading blank line. No formatting is applied.

The footer fires for **both** top-level and per-command help pages — the
`command` argument lets you decide which to show. Common patterns:

- Empty footer everywhere except top-level: check `command.empty()`.
- Different text per command: switch on `command`.
- Marketing/discovery hint to anonymous callers only: gate on app state.

## `man`

Apps expose a built-in `man` command that prints a groff-formatted page to
stdout. Pipe it through `groff`/`mandoc` to preview, or install it under
`/usr/local/share/man/man1/`:

```bash
./myapp man > myapp.1
./myapp man | mandoc                  # preview
sudo install -m 0644 myapp.1 /usr/local/share/man/man1/
man myapp
```

Pass a command name to get a per-command page. Nested subcommands use
dot-separated or space-separated paths:

```bash
./myapp man container > myapp-container.1
./myapp man "container ls" > myapp-container-ls.1
```

The page is derived from the same metadata as `--help`. Hidden and gated
commands are omitted.

The `man` builtin is registered automatically only when your app does not
already define a command with the same name.

### Build-time vs runtime man pages

`man` honours `setEnabledGroups` exactly like `--help`. So:

- Generated from a CI/install script with no caller context → only public
  commands are documented. Right default for distribution, since the man
  page is read by any user including non-staff.
- Generated by an authenticated user (`myapp man > /tmp/myapp.1`) → groups
  enabled at the moment of generation are included.

If you want the distributed man page to always show only public commands,
clear groups before invoking the builtin in your `runCommand` dispatch.

## Shell completion

Apps expose a built-in `completion` command that prints a completion script
for `bash`, `zsh`, or `fish`:

```bash
./myapp completion bash
./myapp completion zsh
./myapp completion fish
```

Typical install:

```bash
# bash (per user)
./myapp completion bash > ~/.local/share/bash-completion/completions/myapp

# zsh (per user; ensure the directory is on $fpath before compinit)
./myapp completion zsh > "${fpath[1]}/_myapp"

# fish (per user)
./myapp completion fish > ~/.config/fish/completions/myapp.fish
```

All three shells complete:

- top-level commands
- subcommands
- per-command options (long and short forms)

Hidden and gated commands are omitted. As with `man`, the `completion`
builtin defers to a user-defined `completion` command if one exists.

### Group-aware completion

Because the visibility filter is the same for help, man, and completion, the
completion script generated by an authenticated user reflects the commands
they can run. Unauthenticated users get a shorter completion list.

If you ship a completion script alongside the binary in a package, generate
it with no groups enabled so the package contains the public surface only;
users can regenerate it locally to pick up additional commands their session
unlocks.
