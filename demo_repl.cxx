#include <iostream>

#include "lua_interpreter.hxx"

using namespace luai;

int main() {
    auto state = lua_interpreter{};
    state.openlibs();

    auto line = std::string{};
    auto linenum = long{1};

    std::cout << "Lua REPL version: " << state.lua_version << "\n\n" << std::flush;
    while (true) {
        std::cout << "in [" << linenum << "] " << std::flush;
    read:
        if (!std::getline(std::cin, line))
            break;
        if (line.size() == 0)
            goto read;
        std::cout << "out [" << linenum << "] " << std::endl;

        auto ret = state.run_chunk(line.c_str());

        // error occured
        if (!std::get<0>(ret))
            std::cerr << "error: " << std::get<1>(ret) << std::endl;

        std::cout << std::endl;
        ++linenum;
    }
}
