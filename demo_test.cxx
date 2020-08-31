#include <stdexcept>
#include <vector>

#include "lua_interpreter.hxx"

#define ASSERT(condition)                                           \
do {                                                                \
    if(!(condition))                                                \
        throw std::runtime_error(std::string( __FILE__ )            \
                                + std::string( ":" )                \
                                + std::to_string( __LINE__ )        \
        );                                                          \
} while (0)

#define SHOULD_THROW(expr)                                          \
do {                                                                \
    bool thrown {false};                                            \
    try {                                                           \
        expr;                                                       \
    } catch (std::runtime_error &) {                                \
        thrown = true;                                              \
    }                                                               \
    ASSERT(thrown);                                                 \
} while (0)

using namespace luai;

// my helper function to get field recursively
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
        "print('this is a test script')\n"
        "x = 15\n"
        "y = x + 16.6\n"
        "s = (function() return 'hahaha' end)()\n"
        "b = true\n"
    );

    ASSERT(std::get<0>(ret) == true);
    ASSERT(state.get_global<types::INT>("x") == 15);
    ASSERT(state.get_global<types::NUM>("y") == 31.6);
    ASSERT(state.get_global<types::STR>("s") == std::string{"hahaha"});
    ASSERT(state.get_global<types::BOOL>("b") == true);
    SHOULD_THROW(state.get_global<types::INT>("xx"));
    // int <-> string, num <-> string are convertible, but int <-> num is not
    SHOULD_THROW(state.get_global<types::INT>("y"));

    state.run_chunk(
        "a = { 1, 6.6, 'haha', false, {} }\n"
    );
    // must open up new scope for tables
    {
        auto a = state.get_global<types::TABLE>("a");
        ASSERT(a.get_index<types::INT>(1) == 1);
        ASSERT(a.get_index<types::NUM>(2) == 6.6);
        ASSERT(a.get_index<types::STR>(3) == std::string{"haha"});
        ASSERT(a.get_index<types::BOOL>(4) == false);
        a.get_index<types::TABLE>(5);
        ASSERT(a.len() == 5);
    }

    state.run_chunk(
        "t = { ['wow'] = 7, ['nest'] = { ['ehh'] = 8, ['more'] = { ['oh'] = 9 } } }\n"
    );
    {
        auto t = state.get_global<types::TABLE>("t");
        ASSERT(t.get_field<types::INT>("wow") == 7);
        {
            auto nest = t.get_field<types::TABLE>("nest");
            ASSERT(nest.get_field<types::INT>("ehh") == 8);
            ASSERT(state.get_global<types::INT>("x") == 15);
            ASSERT(t.get_field<types::INT>("wow") == 7);
        }
        ASSERT(t.get_field<types::INT>("wow") == 7);
    }
    ASSERT(get_field_recur<types::INT>(state, {"t", "nest", "more", "oh"}) == 9);

    state.run_chunk(
        "k = { haha = 8, spam = 8.8, hehe = { wow = 9, kk = { cc = 10, [16] = true } } }\n"
    );
    {
        // test table handle RAII for manipulating stack indices
        // one and one table handle should be defined per scope
        auto k = state.get_global<types::TABLE>("k");
        ASSERT(k.get_field<types::INT>("haha") == 8);
        {
            auto hehe = k.get_field<types::TABLE>("hehe");
            {
                auto kk = hehe.get_field<types::TABLE>("kk");
                ASSERT(kk.get_field<types::INT>("cc") == 10);
                ASSERT(k.get_field<types::INT>("haha") == 8);
                ASSERT(hehe.get_field<types::INT>("wow") == 9);
                SHOULD_THROW(kk.get_field<types::NUM>("spam"));
            }
            ASSERT(hehe.get_field<types::INT>("wow") == 9);
        }
        // don't try to get another table in this scope -- it is undefined
        ASSERT(k.get_field<types::NUM>("spam") == 8.8);
    }
    ASSERT(state.get_global<types::TABLE>("k")
            .get_field<types::TABLE>("hehe")
            .get_field<types::TABLE>("kk")
            .get_index<types::BOOL>(16) == true);

    // move
    auto state2 = std::move(state);
    ASSERT(state2.get_global<types::INT>("x") == 15);
    {
        auto k = state2.get_global<types::TABLE>("k");
        auto k2 = std::move(k);
        ASSERT(k2.get_field<types::NUM>("spam") == 8.8);
    }

    state2.run_chunk(
        "print('bye!')\n"
    );
}
