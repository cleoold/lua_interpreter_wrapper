#pragma once

#include <tuple>
#include <memory>

class lua_interpreter {
public:
    enum class types {
        INT, NUM, STR
    };

    static const int lua_version;

    lua_interpreter();

    // MOVE
    lua_interpreter(lua_interpreter &&) noexcept;
    lua_interpreter &operator=(lua_interpreter &&) noexcept;

    // COPYING DELETED

    ~lua_interpreter();

    // returns whether executing waas successful PLUS error message
    std::tuple<bool, std::string> run_chunk(const char *code) noexcept;

    // opens all standard libraries
    void openlibs() noexcept;

    // get a global variable
    template<types Type>
    std::conditional_t<Type == types::INT, long,
        std::conditional_t<Type == types::NUM, double,
    /*types::STR*/ std::string>>
    get_global_var(const char *varname);

private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};
