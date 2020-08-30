#include "lua.hpp"

#include "lua_interpreter.hxx"

using namespace luai;

// tags
enum class whahaha {
    GLOBAL, TABLE, TABLE_INDEX, FUNC1
};

using LuaInt = long long;

// varwhere -> key type
template<whahaha VarWhere>
using whahaha_key_t =
    std::conditional_t<VarWhere == whahaha::GLOBAL, const char *,
    std::conditional_t<VarWhere == whahaha::TABLE, const char *,
    std::conditional_t<VarWhere == whahaha::TABLE_INDEX, LuaInt,
                                /*FUNC1*/ void (*)(lua_State *, int)
>>>;

namespace {
    // used to build ugly error message
    auto operator+(const std::string &lhs, whahaha_key_t<whahaha::TABLE_INDEX> num) {
        return lhs + std::to_string(num);
    }
    auto operator+(const std::string &lhs, whahaha_key_t<whahaha::FUNC1>) {
        return lhs + "function()";
    }
}

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
    void get_by_name(whahaha_key_t<VarWhere> varname) noexcept;

    // pop 0, push 0
    template<whahaha VarWhere, class R, class Cvrt, class Check, class KeyT = whahaha_key_t<VarWhere>>
    R get_what_impl(KeyT varname, Cvrt &&cvrtfunc, Check &&checkfunc, const char *throwmsg) {
        get_by_name<VarWhere>(varname);
        if (!checkfunc(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error{std::string{"variable/field ["} + varname + "] is not " + throwmsg};
        }
        auto result = cvrtfunc(L, -1, NULL);
        lua_pop(L, 1);
        return {result};
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>, class KeyT = whahaha_key_t<VarWhere>>
    std::enable_if_t<Type == types::INT, R> get_what(KeyT varname) {
        return get_what_impl<VarWhere, R>(varname, lua_tointegerx, lua_isinteger,
            "integer");
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>, class KeyT = whahaha_key_t<VarWhere>>
    std::enable_if_t<Type == types::NUM, R> get_what(KeyT varname) {
        return get_what_impl<VarWhere, R>(varname, lua_tonumberx, lua_isnumber,
            "number or string convertible to number");
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>, class KeyT = whahaha_key_t<VarWhere>>
    std::enable_if_t<Type == types::STR, R> get_what(KeyT varname) {
        return get_what_impl<VarWhere, R>(varname, lua_tolstring, lua_isstring,
            "string or number");
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>, class KeyT = whahaha_key_t<VarWhere>>
    std::enable_if_t<Type == types::BOOL, R> get_what(KeyT varname) {
        static auto toboolean = [](auto ls, auto idx, auto) { return (R)lua_toboolean(ls, idx); };
        // because lua_isboolean is macro
        static auto isboolean = [](auto ls, auto idx) { return lua_isboolean(ls, idx); };
        return get_what_impl<VarWhere, R>(varname, toboolean, isboolean,
            "boolean");
    }

    // pop 0, push 1
    template<whahaha VarWhere, class KeyT = whahaha_key_t<VarWhere>>
    void push_table(KeyT varname) {
        get_by_name<VarWhere>(varname);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error{std::string{"variable "} + varname + " is not table"};
        }
    }

    // pop 1, push 0
    void pop_top_table() noexcept {
        lua_pop(L, 1);
    }

    // assumes table is already on the top of the stack
    // pop 0, push 0
    auto table_len() {
        return get_what_impl<whahaha::FUNC1, LuaInt>(lua_len, lua_tointegerx, lua_isinteger,
            "integer");
    }

    ~impl() {
        if (L)
            lua_close(L);
    }
};

template<>
void lua_interpreter::impl::get_by_name<whahaha::GLOBAL>(whahaha_key_t<whahaha::GLOBAL> varname) noexcept {
    lua_getglobal(L, varname);
}

// assumes table is already on the top of the stack
template<>
void lua_interpreter::impl::get_by_name<whahaha::TABLE>(whahaha_key_t<whahaha::GLOBAL> varname) noexcept {
    lua_getfield(L, -1, varname);
}

// assumes table is already on the top of the stack
template<>
void lua_interpreter::impl::get_by_name<whahaha::TABLE_INDEX>(whahaha_key_t<whahaha::TABLE_INDEX> varidx) noexcept {
    lua_geti(L, -1, varidx);
}

// key is a function
// calls function
template<>
void lua_interpreter::impl::get_by_name<whahaha::FUNC1>(whahaha_key_t<whahaha::FUNC1> f) noexcept {
    f(L, -1);
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

template<types Type>
get_var_t<Type> lua_interpreter::get_global(whahaha_key_t<whahaha::GLOBAL> varname) {
    return pimpl->get_what<whahaha::GLOBAL, Type>(varname);
}
// EXPLICIT INSTANTIATION for basic types
template get_var_t<types::INT> lua_interpreter::get_global<types::INT>(whahaha_key_t<whahaha::GLOBAL>);
template get_var_t<types::NUM> lua_interpreter::get_global<types::NUM>(whahaha_key_t<whahaha::GLOBAL>);
template get_var_t<types::STR> lua_interpreter::get_global<types::STR>(whahaha_key_t<whahaha::GLOBAL>);
template get_var_t<types::BOOL> lua_interpreter::get_global<types::BOOL>(whahaha_key_t<whahaha::GLOBAL>);

template<>
table_handle lua_interpreter::get_global<types::TABLE>(whahaha_key_t<whahaha::GLOBAL> varname) {
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

template<types Type>
get_var_t<Type> table_handle::get_field(whahaha_key_t<whahaha::TABLE> varname) {
    return wpimpl->get_what<whahaha::TABLE, Type>(varname);
}

// EXPLICIT INSTANTIATION for basic types
template get_var_t<types::INT> table_handle::get_field<types::INT>(whahaha_key_t<whahaha::TABLE>);
template get_var_t<types::NUM> table_handle::get_field<types::NUM>(whahaha_key_t<whahaha::TABLE>);
template get_var_t<types::STR> table_handle::get_field<types::STR>(whahaha_key_t<whahaha::TABLE>);
template get_var_t<types::BOOL> table_handle::get_field<types::BOOL>(whahaha_key_t<whahaha::TABLE>);

template<>
table_handle table_handle::get_field<types::TABLE>(whahaha_key_t<whahaha::TABLE> varname) {
    wpimpl->push_table<whahaha::TABLE>(varname);
    return {wpimpl};
}

template<types Type>
get_var_t<Type> table_handle::get_index(whahaha_key_t<whahaha::TABLE_INDEX> idx) {
    return wpimpl->get_what<whahaha::TABLE_INDEX, Type>(idx);
}

// EXPLICIT INSTANTIATION for basic types
template get_var_t<types::INT> table_handle::get_index<types::INT>(whahaha_key_t<whahaha::TABLE_INDEX>);
template get_var_t<types::NUM> table_handle::get_index<types::NUM>(whahaha_key_t<whahaha::TABLE_INDEX>);
template get_var_t<types::STR> table_handle::get_index<types::STR>(whahaha_key_t<whahaha::TABLE_INDEX>);
template get_var_t<types::BOOL> table_handle::get_index<types::BOOL>(whahaha_key_t<whahaha::TABLE_INDEX>);

template<>
table_handle table_handle::get_index<types::TABLE>(whahaha_key_t<whahaha::TABLE_INDEX> idx) {
    wpimpl->push_table<whahaha::TABLE_INDEX>(idx);
    return {wpimpl};
}

LuaInt table_handle::len() {
    return wpimpl->table_len();
}
