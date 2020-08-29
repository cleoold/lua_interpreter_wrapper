#include "lua.hpp"

#include "lua_interpreter.hxx"

struct lua_interpreter::impl {
    lua_State *L;

    impl() {
        auto state = luaL_newstate();
        if (state == NULL)
            throw std::runtime_error{"cannot create lua state: out of memory"};
        L = state;
    }

    impl(impl &&) noexcept = default;
    impl &operator=(impl &&) noexcept = default;

    void openlibs() noexcept {
        luaL_openlibs(L);
    }

    std::tuple<bool, std::string> run_chunk(const char *code) noexcept {
        auto error = luaL_loadstring(L, code) || lua_pcall(L, 0, 0, 0);
        if (error) {
            auto errmsg = lua_tostring(L, -1);
            lua_pop(L, 1); // remove err msg
            return { false, errmsg };
        }
        return { true, {} };
    }

    template<class R, class Cvrt, class Check>
    R get_global_what(const char *varname, Cvrt &&cvrtfunc, Check &&checkfunc, const char *throwmsg) {
        lua_getglobal(L, varname);
        if (!checkfunc(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error{std::string{"variable "} + varname + " is not " + throwmsg};
        }
        auto result = cvrtfunc(L, -1, NULL);
        lua_pop(L, 1);
        return {result};
    }

    ~impl() {
        lua_close(L);
    }
};

const int lua_interpreter::lua_version {LUA_VERSION_NUM};

lua_interpreter::lua_interpreter()
    : pimpl{std::make_unique<lua_interpreter::impl>()}
{}

lua_interpreter::lua_interpreter(lua_interpreter &&) noexcept = default;
lua_interpreter &lua_interpreter::operator=(lua_interpreter &&) noexcept = default;

lua_interpreter::~lua_interpreter() = default;

void lua_interpreter::openlibs() noexcept {
    return pimpl->openlibs();
}

std::tuple<bool, std::string> lua_interpreter::run_chunk(const char *code) noexcept {
    return pimpl->run_chunk(code);
}

template<>
long lua_interpreter::get_global_var<lua_interpreter::types::INT>(const char *varname) {
    return pimpl->get_global_what<long>(varname, lua_tointegerx, lua_isinteger,
        "integer");
}

template<>
double lua_interpreter::get_global_var<lua_interpreter::types::NUM>(const char *varname) {
    return pimpl->get_global_what<double>(varname, lua_tonumberx, lua_isnumber,
        "number or string convertible to number");
}

template<>
std::string lua_interpreter::get_global_var<lua_interpreter::types::STR>(const char *varname) {
    return pimpl->get_global_what<std::string>(varname, lua_tolstring, lua_isstring,
        "string or number");
}
