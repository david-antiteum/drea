# Commands

A command is the action your application performs. Drea reads command
declarations from YAML, parses argv, validates argument counts, dispatches to
your callback, and renders help/completion/man output for free.

## Anatomy

```bash
./myapp container ls --all my-container
       └────┬───┘ └┬┘ └─┬─┘ └─────┬─────┘
       command  sub  opt    argument
```

Drea passes the dotted command path (`container.ls`) to your `run()` callback.
Two subcommands under different parents may share the same leaf name.

## Defining a command

```yaml
commands:
  - command: copy
    description: copy a file
    params-names: src dst
    params: 2
    local-options:
      - force
    global-options:
      - verbose
```

Fields:

| Field            | Required | Meaning |
|------------------|----------|---------|
| `command`        | yes      | Name; unique within a path |
| `description`    | no       | Used by `--help` and `man` |
| `params-names`   | no       | Names of positional args (for help) |
| `params`         | no       | Number of positional args: integer or `unlimited` |
| `min-params`     | no       | Minimum positionals when some are optional. `params` becomes the maximum |
| `local-options`  | no       | Option names that apply only to this command |
| `global-options` | no       | Option names that apply to this command and its subcommands |
| `group`          | no       | One or more groups gating visibility (see *Command groups* below) |
| `commands`       | no       | Nested subcommands |

`parent` is set automatically when commands are nested under `commands:`.

## Subcommands

Nest commands to build a hierarchy:

```yaml
commands:
  - command: container
    description: manage containers
    commands:
      - command: ls
        description: list containers
      - command: rm
        description: remove a container
        params-names: container-id
```

Drea invokes the callback with the full path:

```cpp
app.commander().run( [&]( const std::string & cmd ){
    if( cmd == "container.ls" )      { /* ... */ }
    else if( cmd == "container.rm" ) { /* ... */ }
});
```

## Argument validation

Drea checks the positional argument count against `params` (and `min-params`
when set) **before** invoking your callback. On mismatch it logs an error and
the callback is not called.

```yaml
- command: copy
  params: 2          # exactly two positional args required

- command: tail
  params: unlimited  # any number, including zero

- command: status
  params: 1
  min-params: 0      # zero or one positional arg
```

## Hiding commands at runtime

Set `Command::mHidden = true` to remove a command from `--help`, completion,
"did you mean?" suggestions, and execution. Drea treats hidden commands as
non-existent: invoking one produces the same error as a typo.

```cpp
if( !buildHasFeatureX ) {
    if( auto cmd = app.commander().find( "feature-x" ) ) {
        cmd->mHidden = true;
    }
}
```

`mHidden` is a runtime-only property; there is no YAML key for it. Use it for
reasons that are independent of the caller — build flags, OS, unfinished
features, internal diagnostics. For caller-dependent gating (role, tier,
license), use *groups* instead.

The visibility check happens inside `Commander::run()`. You can flip
`mHidden` any time before calling `run()` (typically after `parse()`), and
all integrations — help, completion, did-you-mean, exec — will honour it.

See [`examples/hidden/`](../examples/hidden) for a runnable sample.

## Command groups

Groups gate a command behind one or more labels. The application enables a
set of groups at runtime; commands declaring a group become visible only when
at least one of their groups is enabled. Drea knows nothing about what the
labels mean — auth, license, beta flag, OS — that is the application's
choice.

A command without a `group` is always visible. A command with `group` set is
hidden (from help, completion, did-you-mean) and refuses execution until one
of its groups is enabled.

### Declaring groups

Single group:

```yaml
- command: rotate-keys
  description: rotate API signing keys
  group: staff-operator
```

Multiple groups (any-of):

```yaml
- command: cost
  description: cost commands
  group: [staff-readonly, beta]
```

Subcommands inherit their parent's groups when they declare none of their
own:

```yaml
- command: cost
  group: staff-readonly
  commands:
    - command: top              # inherits "staff-readonly"
      description: top spenders
    - command: experimental     # explicit override; "staff-readonly" not inherited
      description: experimental
      group: beta
```

Inheritance is computed once, after `parse()` builds the hierarchy, and
recurses through grandchildren.

### Enabling groups at runtime

```cpp
std::vector<std::string> groups;
if( tier >= Tier::ReadOnly ) groups.push_back( "staff-readonly" );
if( tier >= Tier::Operator ) groups.push_back( "staff-operator" );
app.commander().setEnabledGroups( groups );
```

Call this before `Commander::run()`. The default is an empty set, meaning
only commands without groups are visible.

### What "not visible" means

When `Commander::isVisible(cmd)` is false:

- `--help` omits the command (top-level and per-command listings).
- `bash`/`zsh`/`fish` completion omits the command.
- `man` omits the command. A per-command man invocation falls back to the
  top-level page.
- "Did you mean?" suggestions omit the command.
- Direct invocation (`myapp gated-cmd …`) emits the same `Unknown command "X".
  Did you mean "Y"?` error as a typo. The exit-code path is identical, so a
  probe and a mistype are byte-identical on stderr.
- `myapp gated-cmd --help` emits the same unknown-command error rather than
  rendering the command's help page (which would leak its description).

The probe-vs-typo equivalence is intentional: a gated command must be
indistinguishable from one that does not exist.

### When to use groups vs `mHidden`

- **Groups** for caller-dependent visibility — auth tier, license level,
  feature flag opt-in. Declarative in `commands.yml`, drives all integrations
  uniformly.
- **`mHidden`** for unconditional hides that don't depend on the caller —
  unreleased commands, OS-specific commands, engineering-only diagnostics.
  Set in code based on build/environment context.

Both can apply: `mHidden = true` overrides any group check.

See [`examples/groups/`](../examples/groups) for a runnable sample with
`--tier none|readonly|operator`.

## Programmatic command registration

Commands declared in YAML and commands added in code coexist. Either way, the
fields are the same; the YAML parser populates the same `drea::core::Command`
struct.

```cpp
drea::core::Command diag;
diag.mName = "diagnose";
diag.mDescription = "engineering-only";
diag.mHidden = true;
app.commander().add( diag );
```

`add()` returns a `jss::object_ptr<Command>` you can keep to mutate later
(for example to flip `mHidden`).

To drop a command from the registry — typically a builtin (`man`,
`completion`) that the app does not want to expose, or a programmatically
added command that turned out to be unwanted — use
`Commander::remove(dottedName)`. Removing a parent cascades to all its
subcommands:

```cpp
app.commander().remove( "man" );           // drop the man builtin
app.commander().remove( "container" );     // drop "container" and "container.*"
```
