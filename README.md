# lua_interpreter_cpp

Demo project that wraps a little basic Lua API in a C++ manner - object-oriented, RAII, templates. It can execute Lua scripts and extract variables from the Lua state. The C code comes from book _Programming in Lua_ by Roberto Ierusalimschy.

## Dependency
*   Lua >= 5.3

## Build
```sh
cmake CMakeLists.txt
make
```

It will generate a library archive under `build/lib` folder. It also generates two demo executables: `demo_repl`, a Lua REPL basically the same as the built-in one, and `demo`, an executable that shows the result of running `demo.cxx`.

## Example

open up a Lua state, execute some code:

```cpp
using namespace luai;

auto state = lua_interpreter();
state.openlibs(); // load all standard libraries
state.run_chunk("x = 55 y = 66 print(x + y)");
```

Outputs `121` on a new line.

if there are errors, it returns a tuple whose first boolean field is `false` and second field is a string indicating the error, for example

```cpp
auto r = state.run_chunk("print(x + z)");
cout << std::get<1>(r) << endl;
```

Outputs `attempt to perform arithmetic on a nil value (global 'z')` on my machine.

One can grab global variables of type integer, number, string, bool and table. For example, to grab the value of `x`

```cpp
auto x = state.get_global<types::INT>("x"); // 55
```

`std::runtime_error` will be thrown if `x` does not exist (is `nil`) or is other types.

For tables, use a block scope to contain it so the proxy object resets the Lua stack as appropriate when it is destroyed:

```cpp
state.run_chunk("config = {\n"
                "   active = true,\n"
                "   volume = 77.7,\n"
                "   profile = 'normal',\n"
                "   menu = {'roar', 'only my railgun', 'level5 - judglight', 'late in autumn'}\n"
                "}\n"
);
{
    auto tbl = state.get_global<types::TABLE>("config");
    auto active = tbl.get_field<types::BOOL>("active"); // true
    auto volume = tbl.get_field<types::NUM>("volume"); // 77.7
    auto profile = tbl.get_field<types::STR>("profile"); // "normal"

    // open new scope for nested tables
    // this is also an array, so one can use get_index()
    {
        auto menu = tbl.get_field<types::TABLE>("menu");
        auto myvec = std::vector<std::string>();
        auto menu_len = menu.len();
        for (auto i = 1LL; i <= menu_len; ++i)
            myvec.emplace_back(menu.get_index<types::STR>(i));
    }
}
```

## End note

These functions are not thread-safe, though.
