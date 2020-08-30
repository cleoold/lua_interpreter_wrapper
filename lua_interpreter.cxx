#include "lua.hpp"

#include "lua_interpreter.hxx"

using namespace luai;

// tags
enum class whahaha {
    GLOBAL, TABLE
};

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

    // pop 0, push 0
    std::tuple<bool, std::string> run_chunk(const char *code) noexcept {
        auto error = luaL_loadstring(L, code) || lua_pcall(L, 0, 0, 0);
        if (error) {
            auto errmsg = lua_tostring(L, -1);
            lua_pop(L, 1); // remove err msg
            return { false, errmsg };
        }
        return { true, {} };
    }

    // pop 0, push 1
    template<whahaha VarWhere>
    void get_by_name(const char *varname) noexcept;

    // pop 0, push 0
    template<whahaha VarWhere, class R, class Cvrt, class Check>
    R get_what_impl(const char *varname, Cvrt &&cvrtfunc, Check &&checkfunc, const char *throwmsg) {
        get_by_name<VarWhere>(varname);
        if (!checkfunc(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error{std::string{"variable "} + varname + " is not " + throwmsg};
        }
        auto result = cvrtfunc(L, -1, NULL);
        lua_pop(L, 1);
        return {result};
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>>
    std::enable_if_t<Type == types::INT, R> get_what(const char *varname) {
        return get_what_impl<VarWhere, R>(varname, lua_tointegerx, lua_isinteger,
            "integer");
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>>
    std::enable_if_t<Type == types::NUM, R> get_what(const char *varname) {
        return get_what_impl<VarWhere, R>(varname, lua_tonumberx, lua_isnumber,
            "number or string convertible to number");
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>>
    std::enable_if_t<Type == types::STR, R> get_what(const char *varname) {
        return get_what_impl<VarWhere, R>(varname, lua_tolstring, lua_isstring,
            "string or number");
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>>
    std::enable_if_t<Type == types::BOOL, R> get_what(const char *varname) {
        static auto toboolean = [](auto ls, auto idx, auto) { return (R)lua_toboolean(ls, idx); };
        // because lua_isboolean is macro
        static auto isboolean = [](auto ls, auto idx) { return lua_isboolean(ls, idx); };
        return get_what_impl<VarWhere, R>(varname, toboolean, isboolean,
            "boolean");
    }

    // pop 0, push 1
    template<whahaha VarWhere>
    void push_table(const char *varname) {
        get_by_name<VarWhere>(varname);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error{std::string{"variable "} + varname + " is not table"};
        }
    }

    // pop 1, push 0
    void pop_top_table() {
        lua_pop(L, 1);
    }

    ~impl() {
        if (L)
            lua_close(L);
    }
};

template<>
void lua_interpreter::impl::get_by_name<whahaha::GLOBAL>(const char *varname) noexcept {
    lua_getglobal(L, varname);
}

// assumes table is already on the top of the stack
template<>
void lua_interpreter::impl::get_by_name<whahaha::TABLE>(const char *varname) noexcept {
    lua_getfield(L, -1, varname);
}

const int lua_interpreter::lua_version {LUA_VERSION_NUM};

lua_interpreter::lua_interpreter()
    : pimpl{std::make_shared<lua_interpreter::impl>()}
{}

void lua_interpreter::openlibs() noexcept {
    return pimpl->openlibs();
}

std::tuple<bool, std::string> lua_interpreter::run_chunk(const char *code) noexcept {
    return pimpl->run_chunk(code);
}

template<>
get_var_t<types::INT> lua_interpreter::get_global<types::INT>(const char *varname) {
    return pimpl->get_what<whahaha::GLOBAL, types::INT>(varname);
}

template<>
get_var_t<types::NUM> lua_interpreter::get_global<types::NUM>(const char *varname) {
    return pimpl->get_what<whahaha::GLOBAL, types::NUM>(varname);
}

template<>
get_var_t<types::STR> lua_interpreter::get_global<types::STR>(const char *varname) {
    return pimpl->get_what<whahaha::GLOBAL, types::STR>(varname);
}

template<>
get_var_t<types::BOOL> lua_interpreter::get_global<types::BOOL>(const char *varname) {
    return pimpl->get_what<whahaha::GLOBAL, types::BOOL>(varname);
}

template<>
table_handle lua_interpreter::get_global<types::TABLE>(const char *varname) {
    pimpl->push_table<whahaha::GLOBAL>(varname);
    return {pimpl};
}

// must push the table on the top of the stack before constructing
table_handle::table_handle(std::shared_ptr<lua_interpreter::impl> wp)
    : wpimpl{std::move(wp)}
{}

// pops the table out of the stack when destructed
table_handle::~table_handle() {
    if (wpimpl)
        wpimpl->pop_top_table();
}

template<>
get_var_t<types::INT> table_handle::get_field<types::INT>(const char *varname) {
    return wpimpl->get_what<whahaha::TABLE, types::INT>(varname);
}

template<>
get_var_t<types::NUM> table_handle::get_field<types::NUM>(const char *varname) {
    return wpimpl->get_what<whahaha::TABLE, types::NUM>(varname);
}

template<>
get_var_t<types::STR> table_handle::get_field<types::STR>(const char *varname) {
    return wpimpl->get_what<whahaha::TABLE, types::STR>(varname);
}

template<>
get_var_t<types::BOOL> table_handle::get_field<types::BOOL>(const char *varname) {
    return wpimpl->get_what<whahaha::TABLE, types::BOOL>(varname);
}

template<>
table_handle table_handle::get_field<types::TABLE>(const char *varname) {
    wpimpl->push_table<whahaha::TABLE>(varname);
    return {wpimpl};
}
