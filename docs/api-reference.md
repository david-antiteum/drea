# API reference

This page lists the public surface in `include/drea/core/`. Headers can also
be included individually (`#include <drea/core/App.h>`, etc.) or via the
umbrella header `<drea/core/Core>`.

All types live in the `drea::core` namespace.

---

## `App`

The application root. Owns a `Config`, a `Commander`, and a logger.

```cpp
class App {
    explicit App( int argc, char * argv[] );
    ~App();

    static App & instance();

    const std::string & name() const;
    const std::string & description() const;
    const std::string & version() const;
    void setName( std::string_view );
    void setDescription( std::string_view );
    void setVersion( std::string_view );

    Config &       config() const;
    Commander &    commander() const;
    spdlog::logger & logger() const;

    void parse( const std::string & definitions );  // YAML
    void addToParser( std::string_view definitions );
    void parse();

    std::vector<std::string> args() const;          // original argv

    void setHelpFooter( HelpFooterFn fn );
    std::string helpFooter( std::string_view command ) const;

    virtual void configureInRunTime();              // hook for subclasses
};

using HelpFooterFn = std::function<std::string( const App &, std::string_view command )>;
```

### `parse(definitions)`

Parses a YAML document containing `app`, `version`, `description`,
`env-prefix`, `options`, and `commands` blocks. Internally it adds default
options/commands, configures `Config`, sets up the logger, and configures
`Commander`. Call once, after any pre-parse customisation.

### `addToParser(definitions) + parse()`

Two-step variant. Useful when YAML comes from multiple sources. Append all
fragments with `addToParser`, then call `parse()` (no argument) to process.

### `setHelpFooter(fn)`

Installs a callback that returns extra text appended to `--help`. The
callback receives the dotted command path (or empty for top-level help) and
should return the text to print, or an empty string to skip.

### `configureInRunTime()`

Virtual hook. Subclass `App` and override to mutate config/commands before
`parse()` configures everything. Called once, at the start of `parse()`.

---

## `Commander`

The command registry and dispatcher.

```cpp
class Commander {
    Commander & addDefaults();        // registers "completion" and "man"

    jss::object_ptr<Command> add( const Command & cmd );
    std::vector<jss::object_ptr<Command>> add( const std::vector<Command> & );
    void remove( std::string_view cmdName );

    void run( std::function<void( std::string )> f );

    std::vector<std::string> arguments() const;     // post-command argv
    bool empty() const;

    void commands( const std::function<void(const Command&)> & f ) const;
    jss::object_ptr<Command> find( std::string_view cmdName ) const;

    void setEnabledGroups( std::vector<std::string> groups );
    const std::vector<std::string> & enabledGroups() const;
    bool isVisible( const Command & cmd ) const;
    const std::string & requestedCommand() const;

    void unknownCommand( std::string_view command ) const;
    void wrongNumberOfArguments( std::string_view command ) const;
};
```

### `add(cmd)` / `add(cmds)`

Register one or more commands. Returns a stable `jss::object_ptr<Command>`
you can mutate later (for example to flip `mHidden`).

### `remove(cmdName)`

Erase a command (and all its subcommands) from the registry by dotted name.
Useful to drop a builtin (`man`, `completion`) the app does not want to
expose, or to retract a command added programmatically. Removing a parent
cascades to its descendants.

### `run(f)`

Dispatches based on parsed argv:

- `--version`  → prints version and returns
- `--help`     → renders help (top-level or per-command) and returns
- gated cmd    → emits `Unknown command "X". Did you mean "Y"?` and returns
- builtin (`completion`, `man`) → runs builtin and returns
- otherwise    → invokes `f` with the dotted command path

The visibility gate runs first: if the parsed command is not visible
(`!isVisible`), drea emits the unknown-command error and skips the rest.
This makes a probe and a typo indistinguishable.

### `find(cmdName)`

Looks up a command by dotted path. Returns `nullptr` when nothing matches.
The returned `object_ptr` is mutable; use it to set `mHidden`, `mGroups`,
or any other field.

### `setEnabledGroups(groups)`

Sets the list of groups that are currently active. Affects every visibility
check downstream (help, completion, man, did-you-mean, exec gate). Default
is empty: only commands without groups are visible. Call any time before
`run()`.

### `isVisible(cmd)`

True iff the command is not flagged hidden, AND either declares no groups or
declares at least one group that is currently enabled. Used internally and
by integrations; available publicly for custom rendering.

### `requestedCommand()`

The dotted command parsed from argv (`"cost.top"`). Empty when no command
was given. Useful for app-level decisions that need to know what the user
actually typed (analytics, custom dispatch, footer logic).

### `unknownCommand(name)` / `wrongNumberOfArguments(name)`

Emit the standard error messages. The wording matches what drea would emit
on its own; call these from your dispatch when rejecting commands so the UX
stays consistent.

---

## `Config`

Owns the option registry and the parsed values.

```cpp
class Config {
    void setDefaultConfigFile( const std::string & filePath );
    Config & addDefaults();

    void add( const Option & option );
    void add( const std::vector<Option> & options );
    void remove( std::string_view optionName );

    void setEnvPrefix( const std::string & value );

    bool empty() const;
    void options( std::function<void(const Option&)> f ) const;
    jss::object_ptr<Option> find( std::string_view optionName ) const;

    bool used( const std::string & optionName ) const;
    unsigned int intensity( const std::string & optionName ) const;
    void registerUse( const std::string & optionName );

    void set( const std::string & optionName, const std::string & value );
    void append( const std::string & optionName, const std::string & value );

    template<typename T> T get( std::string_view optionName ) const;
    template<typename T> std::vector<T> getAll( std::string_view optionName ) const;

    void reportUnknownArgument( const std::string & optionName ) const;
};
```

`get<T>` valid for `T` ∈ `bool`, `int`, `double`, `std::string`. `intensity`
counts how many times an option appeared on the command line — handy for
`-vvv` patterns.

`registerUse` marks an option as used without setting a value. Useful for
flags whose presence alone matters.

`remove` erases an option by name from the registry and from the "used"
flag list. Subsequent `find()` returns null and any matching CLI flag is
reported as unknown. Typical use: drop a default option (e.g.
`graylog-host`, `config-source`) the app does not want to expose. Override
`App::configureInRunTime` to call `remove` after `addDefaults` runs and
before `configure` reads CLI flags. See
[Configuration → Disabling default options](configuration.md#disabling-default-options).

---

## `Command`

```cpp
struct Command {
    std::string              mName;
    std::string              mParamName;
    std::string              mDescription;
    std::vector<std::string> mLocalParameters;
    std::vector<std::string> mGlobalParameters;
    std::string              mParentCommand;
    int                      mNbParams = 1;
    int                      mMinParams = -1;
    bool                     mHidden = false;
    std::vector<std::string> mGroups;
    static const int         mUnlimitedParams = 0xfffffffa;

    int numberOfParams() const;
    int minParams() const;
    int maxParams() const;
    std::string nameOfParamsForHelp() const;

    // Auto-populated by Commander after parse().
    std::vector<std::string> mSubcommand;
};
```

All fields are public and mutable. The struct is a plain data container; the
behaviour lives in `Commander`.

---

## `Option`

```cpp
struct Option {
    enum class Scope { Both, File, Line, None };

    std::string                 mName;
    std::string                 mParamName;
    std::string                 mDescription;
    std::vector<OptionValue>    mValues;
    std::type_index             mType = typeid( std::string );
    Scope                       mScope = Scope::Both;
    int                         mNbParams = 1;
    std::string                 mShortVersion;
    bool                        mSensitive = false;
    static const int            mUnlimitedParams = 0xfffffffa;

    int numberOfParams() const;
    std::string toString( const OptionValue & val ) const;
    OptionValue fromString( const std::string & val ) const;
    bool helpInLine() const;
    bool helpInFileOnly() const;
};

using OptionValue = std::variant<std::monostate,bool,int,double,std::string>;
```

---

## Umbrella header

`<drea/core/Core>` includes the headers most apps need:

```cpp
#include <drea/core/Core>
```

Pulls in `App`, `Commander`, `Command`, `Config`, `Option`, plus `spdlog`
formatting and the standard library wrappers used by drea.
