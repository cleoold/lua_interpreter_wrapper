#include <iostream>
#include "lua_interpreter.hxx"

using namespace luai;

int main() {
    auto state = lua_interpreter{};
    state.openlibs();
    auto ret = state.run_chunk(
        "x = 15\n"
        "y = x + 16.6\n"
        "s = (function() return 'hahaha' end)()\n"
        "t = { ['wow'] = 7, ['nest'] = { ['ehh'] = 8 } }\n"
        "print(x + y)\n"
        "print(x + s)\n"
    );
    if (!std::get<0>(ret)) {
        std::cerr << "exception: " << std::get<1>(ret) << std::endl;
    }
    std::cout << "x = " << state.get_global<types::INT>("x") << std::endl;
    std::cout << "y = " << state.get_global<types::NUM>("y") << std::endl;
    std::cout << "s = " << state.get_global<types::STR>("s") << std::endl;
    {
        auto tbl = state.get_global<types::TABLE>("t");
        std::cout << "t.wow = " << tbl.get_field<types::INT>("wow") << std::endl;
        {
            auto nest = tbl.get_field<types::TABLE>("nest");
            std::cout << "t.nest.ehh = " << nest.get_field<types::INT>("ehh") << std::endl;
            std::cout << "x = " << state.get_global<types::INT>("x") << std::endl;
        }
    }
}
