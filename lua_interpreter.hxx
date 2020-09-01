#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>

namespace luai {

class luastate_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum class types {
    INT, NUM, STR, BOOL, TABLE,
    NIL, OTHER, LTYPE
};

class table_handle;

// all possible types one can get from state.get_global(),  get_field() and get_index()
template<types Type>
using get_var_t =
    std::conditional_t<Type == types::INT, long long,
    std::conditional_t<Type == types::NUM, double,
    std::conditional_t<Type == types::STR, std::string,
    std::conditional_t<Type == types::BOOL, bool,
    std::conditional_t<Type == types::TABLE, table_handle,
    std::conditional_t<Type == types::LTYPE, types,
    /*unsupported types*/ luastate_error
>>>>>>;

class lua_interpreter {
public:

    static const int lua_version;

    // opens a new lua state
    lua_interpreter();

    // MOVE
    lua_interpreter(lua_interpreter &&) noexcept;
    lua_interpreter &operator=(lua_interpreter &&) noexcept;

    // COPYING DELETED

    // returns whether executing waas successful PLUS error message
    std::tuple<bool, std::string> run_chunk(const char *code) noexcept;

    // opens all standard libraries
    void openlibs() noexcept;

    // get a global variable
    template<types Type>
    get_var_t<Type> get_global(const char *varname);

private:
    struct impl;
    std::shared_ptr<impl> pimpl;

    friend class table_handle;
};

// RAII managed lua table getter
// when this object is alive, the top of the lua stack is always the table.
// when this object is destroyed, the top of the stack is popped
class table_handle {
public:
    // get a field from the current table
    template<types Type>
    get_var_t<Type> get_field(const char *varname);

    // get a field using int index from the current array
    template<types Type>
    get_var_t<Type> get_index(long long idx);

    // get the length of current array. DOES NOT make sense if array contains holes
    // or if __len() metamethod does not return int
    long long len();

    // MOVE
    table_handle(table_handle &&) noexcept;
    table_handle &operator=(table_handle &&) noexcept;

    // COPYING DELETED

    // must be called immediately after using table_handle
    //~table_handle();

private:
    struct impl;
    std::shared_ptr<impl> pimpl;
    table_handle(std::shared_ptr<lua_interpreter::impl>, std::shared_ptr<impl>);

    friend class lua_interpreter;
};

} // namespace luai
