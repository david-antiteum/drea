# Plan: `drea::service` add-on (and supporting core work)

Status: **Phases 1–2 done** (A1 + A2 released as 0.33.0; A3–A7 landed
2026-07-06, see CHANGELOG Unreleased — profile-service can now delete its
local `CorrelationId.h`, flush workaround and validation code). Written
2026-07-06. This document is the contract for the remaining phases (3–7).

## Positioning

Drea's identity stays what it is: a framework for C++ CLI applications in the
spirit of Go's **Cobra** (commands) and **Viper** (config sources). Nothing in
this plan changes that, and the core keeps its current dependency footprint.

Service support arrives as an **add-on module**, not a core feature. It is
deliberately **opinionated** — it supports exactly one stack and does it well:

| Concern | Choice (no alternatives offered) |
|---|---|
| Transport | gRPC over TLS (sync API today; callback API is the upgrade path) |
| Storage | PostgreSQL (libpqxx) |
| Auth | JWT bearer tokens, RS256, verified against an OIDC JWKS endpoint (Cognito-shaped) |
| Telemetry | Sentry (sentry-native): errors + traces + metrics |
| Health | `grpc.health.v1` |

A team wanting REST, MySQL, or OTel does not use this module. That is the
point: every Aimsun service converges on one runtime shape, and the checklist
(`backend-sample/docs/cpp-service-checklist.md`) is enforced by construction
instead of by review.

The reference consumer is the existing sample service at
`~/development/projects/backend-sample` (profile-service). Most of Part B below
is **extraction**, not invention — the code exists there, works, and is tested;
it moves here and the sample becomes the module's first consumer.

## Architecture decisions

- **Packaging**: new vcpkg feature `service` (same pattern as the existing
  `aws` / `toml` features) + a separate CMake target `drea-service` exporting
  `drea::service`. Core target unchanged; CLI users see zero new dependencies.
- **New dependencies (feature-gated, never core)**: `grpc`, `protobuf`,
  `jwt-cpp`, `libpqxx`, `sentry-native`, `openssl`. No `cpr`: the JWKS fetch
  (B5) reuses drea's own HTTP client (`core/utilities/httpclient.h`,
  boost-beast — already used by the graylog/consul/etcd integrations) and
  nlohmann-json, both already core dependencies.
- **C++ standard**: core stays **C++17** (CLI consumers may lag). The
  `drea-service` target requires **C++20** (`jthread`, `stop_token`,
  `source_location`). Two standards, one repo — set per target, document it.
- **Namespace**: `drea::service`. Headers under `include/drea/service/`.
- **Sources layout**: `service/` sibling of `core/` (own CMakeLists, own tests).

---

## Part A — core improvements (benefit CLI apps too, land first)

These fix real bugs or add Viper-adjacent features. Each is a small independent
PR against core. No new dependencies.

### A1. Logger flush policy  *(bug, found 2026-07-06)*
`Config::setupLogger()` installs no flush policy: with `--log-folder` the
rotating file sink buffers and the log file stays empty until process exit.
Fix: `flush_on(spdlog::level::warn)` + `spdlog::flush_every(3s)` as defaults,
plus a `log-flush-level` predefined option to override. (profile-service works
around this today with `flush_on(info)` in its `main`; delete the workaround
once this lands.)

### A2. JSON formatter on the file sink
House rule is "JSON (structured) to file; human text to console" — today both
sinks get the same text pattern. Give the file sink a JSON-lines formatter
(fields: ISO 8601 timestamp with timezone, `level`, `logger`, `msg`) and keep
the colored text pattern on the console sink. `msg` must be JSON-escaped — do
this with a real formatter (custom `spdlog::formatter`), not a `set_pattern`
string, or embedded quotes corrupt the stream. Scope the formatter to the file
sink only: the graylog sink does its own GELF serialization and must not have
the JSON formatter applied on top of it.

### A3. `drea::log::redacted()` helper
Runtime counterpart of the existing `sensitive: true` option flag. Wrap any
value that must not reach production logs:

```cpp
app.logger().debug( "user email {}", drea::log::redacted( email ) );
```

- Small view-holding struct + `fmt::formatter` specialization: prints
  `[redacted]` when redaction is on, the value otherwise. Zero allocation.
- Driven by a predefined option (`log-redact`, default **on**; dev turns it
  off), read once at startup and frozen — config is immutable per house rules.
- Document the redact-by-default field list (see the checklist's Logging
  section: identifiers, credentials, network identifiers, free text).

### A4. `drea::log::sanitizeCorrelationId()`
Client-supplied values that end up in log lines (request ids, session ids) must
be clamped: charset `[0-9A-Za-z._-]`, max 64 chars, else empty. Pure-std,
header-only. Extract verbatim from
`backend-sample/services/profile/src/CorrelationId.h` (tests exist there too).

### A5. Declarative option validation
commands.yml grows optional keys: `required: true`, `min:`, `max:` (numeric).
`App::parse` validates after source resolution and fails with a clear message
+ config exit code. Deletes the hand-written "is pool-id set / is port in
range" blocks every service and CLI currently writes in `main`.

### A6. Log the effective config automatically
After parse, emit one `info` line per option with its resolved value and
source (flag/env/file/default), sensitive values redacted. Today each app
hand-writes an incomplete version of this. Predefined option to suppress.

### A7. Standard exit codes
`enum class drea::core::ExitCode { Ok = 0, ConfigError = 78, DependencyError = 69, ... }`
(sysexits-inspired). Used by A5's validation failures; services and CLIs adopt
the same vocabulary so orchestrators and runbooks read one table.

---

## Part B — the `drea::service` module

Target shape of a service `main` once this exists:

```cpp
int main( int argc, char * argv[] )
{
	drea::core::App app( argc, argv );
	app.parse( kCommandsYml );                     // includes the standard service fragment

	return app.commander().run( [ & ]() -> int {
		drea::service::Runtime runtime( app );      // Sentry, TLS, limits, health — from config

		Database    db( runtime );                  // B8: pool, fail-fast open
		ServiceImpl impl( db, app.logger() );       // the only per-service code

		runtime.auth().open( "/profile.v1.ProfileService/Ping" );
		return runtime.serve( { &impl } );          // B1: signals, drain, shutdown deadline
	} );
}
```

### B1. Server runtime + graceful lifecycle
Extract `Service::run()` from the sample: signal mask before thread creation,
`sigwait` waiter thread, readiness flip to NOT_SERVING, drain grace, then
`Shutdown(deadline)`; flush sinks; return typed exit codes (A7). Configurable
knobs (drain grace, shutdown window) come from the standard options fragment.

### B2. Health service
`grpc.health.v1` implementation with a readiness callback
(`std::function<bool()>`), registered automatically by B1. Vendored
`health.proto` moves here.

### B3. Request context + request-id propagation
Extract `RequestContext` (+ its `makeRequestContext`): reuse a valid inbound
`x-request-id` (Envoy generates one), else mint a UUID; echo it back as
response metadata; sanitize (A4) all client-supplied correlation values.

### B4. `RpcGuard` — per-RPC access log + Sentry transaction
Extract from the sample's `ServiceImpl.cpp`. One RAII object per handler owning
the two things that must happen on every exit path: the Sentry transaction
(tagged request_id/session_id/sub, `grpc.status`) and the access line
(`info` on OK, `warn` + error detail otherwise; fields `rpc`, `request_id`,
`session_id`, `sub`, `src`, `status`, `duration_ms`). Includes
`statusCodeName()`. This is the checklist's post-mortem-readiness section as
code.

### B5. JWT auth
Extract `Token` (JWKS fetch + RS256 verify + issuer/audience/`token_use`/expiry
checks, jwt-cpp; JWKS fetched with drea's `utilities/httpclient.h`, replacing
the sample's cpr usage) and `AuthProcessor` (bearer or `token` metadata key,
verified-identity cache with double expiry, identity published to the
AuthContext, **rejections logged locally** — a rejected RPC never reaches a
handler, so this is the only local trace). Parameterize: issuer URL, audience,
expected `token_use`, open links, identity property names. Cognito is the
tested profile; anything OIDC/JWKS-shaped works by construction.

### B6. Server limits from config
Standard options applied to the `ServerBuilder`: max send/receive message
size, max concurrent streams, keepalive, `MAX_CONNECTION_AGE(+grace)`.
Opinionated defaults; all overridable via the fragment.

### B7. Sentry runtime
Extract `SentryGuard`: init from `sentry.dsn` (no-op when empty), release
string, `traces_sampler` with head-based per-route sampling (not a flat
`1.0`), metrics enabled. Owned by B1's `Runtime`.

### B8. Postgres support
Extract and generalize from the sample's `Database`:
- keyword/value connection-string builder with correct quoting (the sample's
  `quote()`/`buildConnectionString`),
- bounded connection pool with deadline-aware acquire
  (min(inbound RPC deadline, local cap) — the sample's `poolDeadline`),
- `healthy()` hook wired into B2 readiness,
- fail-fast `open()` at startup,
- schema-version check at startup (versioned, forward-only migrations; abort
  on mismatch — checklist Failure-handling).

### B9. Resource-gauge ticker
Extract the `jthread` + `stop_token` periodic reporter: caller registers
`(name, unit, callback)` gauges; module reports them through Sentry metrics.

### B10. Standard options fragment
A commands.yml snippet the module ships (`service.address/port/max-concurrent`,
`tls.cert/key`, `database.*` incl. `pool-size` and sensitive `password`,
`sentry.dsn` sensitive, drain/shutdown windows, B6 limits). Apps compose it
into their own yml — mechanism to be decided (include directive vs. string
concatenation at `parse`; see Open questions).

### Out of scope (per service, forever)
Proto contracts and generated code, RPC handlers, SQL schema and queries,
which RPCs are open, the `Ping`/version RPC (message type lives in each
service's proto package — share the CMake `version.h` generation pattern via
`cmake/`, not library code), Envoy/deploy config.

---

## Phasing

Each phase is releasable on its own; A-items are independent small PRs.

1. **Core fixes**: A1, A2 (the two live logging bugs) → patch/minor release.
2. **Core features**: A3, A4, A5, A6, A7 → minor release. profile-service
   deletes its local `CorrelationId.h`, flush workaround, and validation code.
3. **Module skeleton**: vcpkg feature + `drea-service` target + B1, B2, B3,
   B6, B7, B10. A gutted copy of profile-service builds against it.
4. **Observability**: B4, B9.
5. **Auth**: B5 (biggest API-design surface — do it once the runtime shape is
   settled).
6. **Postgres**: B8.
7. **Migration proof**: profile-service drops every extracted file; its diff
   is the acceptance test. Target: its `src/` reduced to `ServiceImpl`,
   `Database` (queries only), `main`, protos.

## Testing

- Part A: Catch2 unit tests in `tests/` as today (formatter output, redaction
  on/off, sanitizer, validation failures, exit codes).
- Part B: unit tests for pure parts (sanitize, status names, deadline math,
  token verification against fixed JWKS fixtures); the migrated profile-service
  is the integration test (its existing grpcurl/test-client flows must pass
  unchanged).
- CI: keep the module honest under ASAN/UBSAN and TSAN presets (B1 is
  thread-heavy).

## Open questions (answer before Phase 3)

1. **Fragment composition** (B10): yml `include:` directive in Drea vs.
   compile-time string concatenation of the fragment header. Leaning:
   concatenation first (no parser change), directive later if it hurts.
2. **`Runtime` granularity**: one façade object (sketch above) vs. separate
   `SentryGuard`/`GrpcServer`/`AuthProcessor` the app wires manually. Leaning:
   façade with accessors — opinionated module, opinionated wiring.
3. **Sync vs. callback gRPC API**: extract sync (what exists, what the sample
   uses) and keep the callback API as a later internal upgrade of B1/B4 — the
   module boundary should not expose which one is underneath.
4. **sentry-native metrics model**: trace-metrics superseded the statsd-style
   API upstream; B9 should target the current API of the pinned version at
   implementation time — verify before coding.
5. **Repo vs. separate repo**: this plan assumes in-repo (`service/` dir +
   feature). If Drea-as-CLI purity wins the argument, the same plan applies to
   a `drea-service` sibling repo depending on Drea; only packaging changes.
