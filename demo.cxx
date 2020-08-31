#include <iostream>
#include <vector>

#include "lua_interpreter.hxx"

using namespace luai;

// len of names must >= 1
template<types Type>
auto get_field_recur(lua_interpreter &state, const std::vector<std::string> &names) {
    if (names.size() == 1)
        return state.get_global<Type>(names[0].c_str());
    auto staq = std::vector<table_handle>{};
    staq.emplace_back(state.get_global<types::TABLE>(names[0].c_str()));
    for (size_t i = 1; i < names.size()-1; ++i)
        staq.emplace_back(staq[i-1].get_field<types::TABLE>(names[i].c_str()));
    return staq.back().get_field<Type>(names.back().c_str());
    // table handles destructed
}

int main() {
    auto state = lua_interpreter{};
    state.openlibs();
    auto ret = state.run_chunk(
        "x = 15\n"
        "y = x + 16.6\n"
        "s = (function() return 'hahaha' end)()\n"
        "t = { ['wow'] = 7, ['nest'] = { ['ehh'] = 8, ['more'] = { ['oh'] = 9 } } }\n"
        "a = { 'axx', 'byy', 'czz' }\n"
        "print(x + y)\n"
        "print(x + s)\n"
    );
    if (!std::get<0>(ret)) {
        std::cerr << "exception: " << std::get<1>(ret) << std::endl;
    }
    std::cout << "x = " << state.get_global<types::INT>("x") << std::endl;
    std::cout << "y = " << state.get_global<types::NUM>("y") << std::endl;
    std::cout << "s = " << state.get_global<types::STR>("s") << std::endl;
    // must open up new scope for tables
    {
        auto tbl = state.get_global<types::TABLE>("t");
        std::cout << "t.wow = " << tbl.get_field<types::INT>("wow") << std::endl;
        {
            auto nest = tbl.get_field<types::TABLE>("nest");
            std::cout << "t.nest.ehh = " << nest.get_field<types::INT>("ehh") << std::endl;
            std::cout << "x = " << state.get_global<types::INT>("x") << std::endl;
            std::cout << "t.wow = " << tbl.get_field<types::INT>("wow") << std::endl;
        }
        std::cout << "t.wow = " << tbl.get_field<types::INT>("wow") << std::endl;
    }
    // convenient
    std::cout << "t.nest.more.oh = " << get_field_recur<types::INT>(state, {"t", "nest", "more", "oh"}) << std::endl;

    {
        auto arr = state.get_global<types::TABLE>("a");
        auto len = arr.len();
        for (auto i = 1LL; i <= len; ++i)
            std::cout << "a[" << i << "] = " << arr.get_index<types::STR>(i) << std::endl;
    }
}
