# lua_interpreter_cpp

Demo project that wraps a little basic Lua API in a C++ manner - object-oriented, RAII, templates. It can execute Lua scripts and extract variables from the Lua state. The C code comes from book _Programming in Lua_ by Roberto Ierusalimschy.

## Dependency
*   Lua >= 5.3

## Build
```sh
cmake CMakeLists.txt
make
make test
```

It will generate a library archive under `build/lib` folder. It also generates two demo executables: `demo_repl`, a Lua REPL basically the same as the built-in one, and `demo_test`, an executable that shows the result of running `demo_test.cxx`.

## Example

### Globals

open up a Lua state, execute some code:

```cpp
using namespace luai;

auto state = lua_interpreter();
state.openlibs(); // load all standard libraries
state.run_chunk("x = 55 y = 66 f = 6.6 s = 'hehe' print(x + y)");
```

Outputs `121` on a new line.

if there are errors, it returns a tuple whose first boolean field is `false` and second field is a string indicating the error, for example

```cpp
auto r = state.run_chunk("print(x + z)");
cout << std::get<1>(r) << endl;
```

Outputs `attempt to perform arithmetic on a nil value (global 'z')` on my machine.

One can grab global variables of type integer, number, string, bool and table (they are defined as `enum class types;` in `lua_interpreter.hxx`). For example:

```cpp
auto x = state.get_global<types::INT>("x"); // 55
auto f = state.get_global<types::NUM>("f"); // 6.6
auto s = state.get_global<types::STR>("s"); // "hehe"
```

`luastate_error` will be thrown if variable does not exist (is `nil`) or is other types.

The types are introspectible by passing special template parameter `types::LTYPE` to `get_global` method:

```cpp
auto xtype = state.get_global<types::LTYPE>("x"); // types::INT
auto ftype = state.get_global<types::LTYPE>("f"); // types::NUM (floats)
auto ztype = state.get_global<types::LTYPE>("z"); // types::NIL (variable does not exist)
```

### Tables

For tables, getting a `types::TABLE` returns a `table_handle`. When this object is constructed, the corresponding table is pushed to the Lua stack so we can use `get_field` to obtain its fields (whose signature is the same as previous `get_global`). When this object is destructed, it removes that table from the lua Stack. Use a block scope to contain the returned object so it resets the Lua stack as appropriate when it is destroyed:

```cpp
state.run_chunk("config = {\n"
                "   active = true,\n"
                "   volume = 77.7,\n"
                "   profile = 'normal',\n"
                "   menu = {'roar', 'only my railgun', 'level5 - judglight', 'late in autumn'}\n"
                "}\n"
);
{
    // define new table handles only in the scope
    auto tbl = state.get_global<types::TABLE>("config"); // handle
    // config pushed to Lua stack
    auto active = tbl.get_field<types::BOOL>("active"); // true
    auto volume = tbl.get_field<types::NUM>("volume"); // 77.7
    auto profile = tbl.get_field<types::STR>("profile"); // "normal"

    // open new scope for nested tables
    // this is also an array, so one can use get_index()
    {
        auto menu = tbl.get_field<types::TABLE>("menu");
        // menu pushed to Lua stack
        auto myvec = std::vector<std::string>();
        auto menu_len = menu.len();
        for (auto i = 1LL; i <= menu_len; ++i)
            myvec.emplace_back(menu.get_index<types::STR>(i));
    }
    // menu popped from Lua stack
}
// config popped from Lua stack
```

So do not try to move the `table_handle` to containers, threads, etc that live longer than its current scope - it breaks RAII. `demo_test` can be referred to for detailed usage.

By taking the advantage of object destruction order, `table_handle`s need not be nested:

```cpp
{
    auto config = state.get_global<types::TABLE>("config");
    auto menu = config.get_field<types::TABLE>("menu");
    std::cout << "now playing - " << menu.get_index<types::STR>(1)
              << " 🔊" << config.get_field<types::NUM>("volume")
              << std::endl;
}
```

However, that beginning scope block is still needed. This prints `now playing - roar  🔊77.7` on a new line.

## End note

These functions are not thread-safe, though. Use a mutex lock to ensure sync.
