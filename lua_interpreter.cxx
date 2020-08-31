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
    constexpr int IGNORED {};

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
            throw luastate_error{"cannot create lua state: out of memory"};
        L = state;
    }

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
    void get_by_key(whahaha_key_t<VarWhere> key, int tidx);

    // grab value found by "key" based on the table at index "tidx"
    // if VarWhere is GLOBAL then tidx should be ignored
    // pop 0, push 0
    template<whahaha VarWhere, class R, class Cvrt, class Check, class KeyT = whahaha_key_t<VarWhere>>
    R get_what_impl(KeyT key, int tidx, Cvrt &&cvrtfunc, Check &&checkfunc, const char *throwmsg) {
        get_by_key<VarWhere>(key, tidx);
        if (!checkfunc(L, -1)) {
            lua_pop(L, 1);
            throw luastate_error{std::string{"variable/field ["} + key + "] is not " + throwmsg};
        }
        auto result = cvrtfunc(L, -1, NULL);
        lua_pop(L, 1);
        return {result};
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>, class KeyT = whahaha_key_t<VarWhere>>
    std::enable_if_t<Type == types::INT, R> get_what(KeyT key, int tidx) {
        return get_what_impl<VarWhere, R>(key, tidx, lua_tointegerx, lua_isinteger,
            "integer");
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>, class KeyT = whahaha_key_t<VarWhere>>
    std::enable_if_t<Type == types::NUM, R> get_what(KeyT key, int tidx) {
        return get_what_impl<VarWhere, R>(key, tidx, lua_tonumberx, lua_isnumber,
            "number or string convertible to number");
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>, class KeyT = whahaha_key_t<VarWhere>>
    std::enable_if_t<Type == types::STR, R> get_what(KeyT key, int tidx) {
        return get_what_impl<VarWhere, R>(key, tidx, lua_tolstring, lua_isstring,
            "string or number");
    }

    // PARTIAL SPECIALIZATIONS
    // calls get_what_impl(), pop 0, push 0
    template<whahaha VarWhere, types Type, class R = get_var_t<Type>, class KeyT = whahaha_key_t<VarWhere>>
    std::enable_if_t<Type == types::BOOL, R> get_what(KeyT key, int tidx) {
        static auto toboolean = [](auto ls, auto idx, auto) { return (R)lua_toboolean(ls, idx); };
        // because lua_isboolean is macro
        static auto isboolean = [](auto ls, auto idx) { return lua_isboolean(ls, idx); };
        return get_what_impl<VarWhere, R>(key, tidx, toboolean, isboolean,
            "boolean");
    }

    // pop 0, push 1
    template<whahaha VarWhere, class KeyT = whahaha_key_t<VarWhere>>
    void push_table(KeyT key, int tidx) {
        get_by_key<VarWhere>(key, tidx);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            throw luastate_error{std::string{"variable/field ["} + key + "] is not table"};
        }
    }

    // pop 0, push 0
    int get_top_idx() noexcept {
        return lua_gettop(L);
    }

    // rotate, 1 removed overall
    void remove_table(int idx) noexcept {
        lua_remove(L, idx);
    }

    // assumes table is already in the stack at index tidx
    // pop 0, push 0
    auto table_len(int tidx) {
        return get_what_impl<whahaha::FUNC1, LuaInt>(lua_len, tidx, lua_tointegerx, lua_isinteger,
            "integer");
    }

    void protect_indexing(int idx) {
        if (!(get_top_idx() >= idx))
            throw luastate_error{"Malformed Lua stack indexing"};
    }

    ~impl() {
        if (L)
            lua_close(L);
    }
};

// int param is ignored
template<>
void lua_interpreter::impl::get_by_key<whahaha::GLOBAL>(whahaha_key_t<whahaha::GLOBAL> keyname, int) {
    lua_getglobal(L, keyname);
}

// assumes table is already on the stack at index tidx
template<>
void lua_interpreter::impl::get_by_key<whahaha::TABLE>(whahaha_key_t<whahaha::GLOBAL> keyname, int tidx) {
    protect_indexing(tidx);
    lua_getfield(L, tidx, keyname);
}

// assumes table is already on the stack at index tidx
template<>
void lua_interpreter::impl::get_by_key<whahaha::TABLE_INDEX>(whahaha_key_t<whahaha::TABLE_INDEX> keyidx, int tidx) {
    protect_indexing(tidx);
    lua_geti(L, tidx, keyidx);
}

// key is a function
// calls function
template<>
void lua_interpreter::impl::get_by_key<whahaha::FUNC1>(whahaha_key_t<whahaha::FUNC1> f, int tidx) {
    protect_indexing(tidx);
    f(L, tidx);
}

const int lua_interpreter::lua_version {LUA_VERSION_NUM};

lua_interpreter::lua_interpreter()
    : pimpl{std::make_shared<lua_interpreter::impl>()}
{}

lua_interpreter::lua_interpreter(lua_interpreter &&) noexcept = default;
lua_interpreter &lua_interpreter::operator=(lua_interpreter &&) noexcept = default;

void lua_interpreter::openlibs() noexcept {
    return pimpl->openlibs();
}

std::tuple<bool, std::string> lua_interpreter::run_chunk(const char *code) noexcept {
    return pimpl->run_chunk(code);
}

template<types Type>
get_var_t<Type> lua_interpreter::get_global(whahaha_key_t<whahaha::GLOBAL> varname) {
    return pimpl->get_what<whahaha::GLOBAL, Type>(varname, IGNORED);
}
// EXPLICIT INSTANTIATION for basic types
template get_var_t<types::INT> lua_interpreter::get_global<types::INT>(whahaha_key_t<whahaha::GLOBAL>);
template get_var_t<types::NUM> lua_interpreter::get_global<types::NUM>(whahaha_key_t<whahaha::GLOBAL>);
template get_var_t<types::STR> lua_interpreter::get_global<types::STR>(whahaha_key_t<whahaha::GLOBAL>);
template get_var_t<types::BOOL> lua_interpreter::get_global<types::BOOL>(whahaha_key_t<whahaha::GLOBAL>);

template<>
table_handle lua_interpreter::get_global<types::TABLE>(whahaha_key_t<whahaha::GLOBAL> varname) {
    pimpl->push_table<whahaha::GLOBAL>(varname, IGNORED);
    return {pimpl, nullptr};
}

struct table_handle::impl {
    std::shared_ptr<lua_interpreter::impl> pstate;
    // own a reference to the parent impl to avoid popping stack even if parent itself is freed
    std::shared_ptr<impl> parent;
    // where is the current table on the stack
    int stack_index;

    // creation assumes table is already on the top of the stack
    // currently the creation of table handle is managed by get_global, get_field, get_index functions,
    // which takes care of pushing
    // the destruction of table handle is managed by the destructor of this class
    // beware
    impl(std::shared_ptr<lua_interpreter::impl> &&interp_impl, std::shared_ptr<impl> &&parent_impl)
        : pstate{std::move(interp_impl)}, parent{std::move(parent_impl)}
        , stack_index{pstate->get_top_idx()}
    {}

    ~impl() {
        // technically 2nd condition is false only if user uses function incorrectly we
        // have to crash program
        if (pstate && pstate->get_top_idx() >= stack_index)
            pstate->remove_table(stack_index);
    }
};

// must push the table on the top of the stack before constructing
table_handle::table_handle(std::shared_ptr<lua_interpreter::impl> interp_impl, std::shared_ptr<impl> parent_impl)
    : pimpl{std::make_shared<impl>(std::move(interp_impl), std::move(parent_impl))}
{}

table_handle::table_handle(table_handle &&) noexcept = default;
table_handle &table_handle::operator=(table_handle &&) noexcept = default;

template<types Type>
get_var_t<Type> table_handle::get_field(whahaha_key_t<whahaha::TABLE> varname) {
    return pimpl->pstate->get_what<whahaha::TABLE, Type>(varname, pimpl->stack_index);
}

// EXPLICIT INSTANTIATION for basic types
template get_var_t<types::INT> table_handle::get_field<types::INT>(whahaha_key_t<whahaha::TABLE>);
template get_var_t<types::NUM> table_handle::get_field<types::NUM>(whahaha_key_t<whahaha::TABLE>);
template get_var_t<types::STR> table_handle::get_field<types::STR>(whahaha_key_t<whahaha::TABLE>);
template get_var_t<types::BOOL> table_handle::get_field<types::BOOL>(whahaha_key_t<whahaha::TABLE>);

template<>
table_handle table_handle::get_field<types::TABLE>(whahaha_key_t<whahaha::TABLE> varname) {
    pimpl->pstate->push_table<whahaha::TABLE>(varname, pimpl->stack_index);
    return {pimpl->pstate, pimpl};
}

template<types Type>
get_var_t<Type> table_handle::get_index(whahaha_key_t<whahaha::TABLE_INDEX> idx) {
    return pimpl->pstate->get_what<whahaha::TABLE_INDEX, Type>(idx, pimpl->stack_index);
}

// EXPLICIT INSTANTIATION for basic types
template get_var_t<types::INT> table_handle::get_index<types::INT>(whahaha_key_t<whahaha::TABLE_INDEX>);
template get_var_t<types::NUM> table_handle::get_index<types::NUM>(whahaha_key_t<whahaha::TABLE_INDEX>);
template get_var_t<types::STR> table_handle::get_index<types::STR>(whahaha_key_t<whahaha::TABLE_INDEX>);
template get_var_t<types::BOOL> table_handle::get_index<types::BOOL>(whahaha_key_t<whahaha::TABLE_INDEX>);

template<>
table_handle table_handle::get_index<types::TABLE>(whahaha_key_t<whahaha::TABLE_INDEX> idx) {
    pimpl->pstate->push_table<whahaha::TABLE_INDEX>(idx, pimpl->stack_index);
    return {pimpl->pstate, pimpl};
}

LuaInt table_handle::len() {
    return pimpl->pstate->table_len(pimpl->stack_index);
}
