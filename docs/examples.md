# Examples

All samples live under [`examples/`](../examples). Build with `BUILD_EXAMPLES=ON`
(default) and find the binaries in `build/bin/`.

| Example | What it shows |
|---|---|
| [`hello`](../examples/hello) | Bare-minimum app skeleton. No commands, just description and version. |
| [`saythis`](../examples/saythis) | Single command (`this <arg>`) with one local option (`--reverse`). The shortest meaningful drea program. |
| [`say`](../examples/say) | Nested subcommands, three levels deep (`repeat parrot blue`), inherited global options. |
| [`calculator`](../examples/calculator) | Multiple commands (`sum`, `power`, `count`), typed options (`int`, `double`), env-prefix (`CAL_*`), `params: unlimited`, short forms. |
| [`multi`](../examples/multi) | Repeated options with `params: 1, scope: line`. Read all values via `Config::getAll<T>`. |
| [`hidden`](../examples/hidden) | `Command::mHidden` — runtime-only hide based on a CLI flag (`--dev`). Demonstrates programmatic command registration alongside YAML. |
| [`groups`](../examples/groups) | Command groups (tier-style gating), `setEnabledGroups`, dynamic help footer for anonymous callers. Mirrors the ABO-334 myaimsun pattern. |
| [`aws-secrets`](../examples/aws-secrets) | Loading config from AWS Secrets Manager via `--config-source aws://…`. Requires `-DENABLE_AWS=ON` and the `aws` vcpkg feature. |

## Running

```bash
cmake --preset debug
cmake --build --preset debug
./build/bin/saythis this hello
```

## What to read first

- New to drea: start with `saythis`, then `calculator` for typed options.
- Building a multi-command app: study `say` for hierarchy and global options.
- Adding role/tier gating: `groups` is the canonical reference.
- Loading secrets: `aws-secrets`.

Each example is self-contained: a `CMakeLists.txt`, a `commands.yml`, and a
single `main.cpp`. Use them as a starting template by copying the directory
into your own project.
