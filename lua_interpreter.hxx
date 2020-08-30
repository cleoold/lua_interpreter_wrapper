#pragma once

#include <tuple>
#include <memory>

namespace luai {

enum class types {
    INT, NUM, STR, BOOL, TABLE
};

class table_handle;

template<types Type>
using get_var_t =
    std::conditional_t<Type == types::INT, long long,
    std::conditional_t<Type == types::NUM, double,
    std::conditional_t<Type == types::STR, std::string,
    std::conditional_t<Type == types::BOOL, bool,
                    /*types::TABLE*/ table_handle
>>>>;

class lua_interpreter {
public:

    static const int lua_version;

    // opens a new lua state
    lua_interpreter();

    // MOVE
    lua_interpreter(lua_interpreter &&) noexcept = default;
    lua_interpreter &operator=(lua_interpreter &&) noexcept = default;

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
    table_handle(table_handle &&) noexcept = default;
    table_handle &operator=(table_handle &&) noexcept = default;

    // COPYING DELETED

    // must be called immediately after using table_handle
    ~table_handle();

private:
    table_handle(std::shared_ptr<lua_interpreter::impl>);
    std::shared_ptr<lua_interpreter::impl> wpimpl;

    friend class lua_interpreter;
};

} // namespace luai
