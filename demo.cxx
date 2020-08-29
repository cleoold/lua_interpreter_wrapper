#include <iostream>
#include "lua_interpreter.hxx"

int main() {
    auto state = lua_interpreter{};
    state.openlibs();
    auto ret = state.run_chunk(
        "x = 15\n"
        "y = 16.6\n"
        "s = (function() return 'hahaha' end)()\n"
        "print(x + y)\n"
        "print(x + s)\n"
    );
    if (!std::get<0>(ret)) {
        std::cerr << "exception: " << std::get<1>(ret) << std::endl;
    }
    std::cout << state.get_global_var<lua_interpreter::types::INT>("x") << std::endl;
    std::cout << state.get_global_var<lua_interpreter::types::NUM>("y") << std::endl;
    std::cout << state.get_global_var<lua_interpreter::types::STR>("s") << std::endl;
}
