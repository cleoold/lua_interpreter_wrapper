cmake_minimum_required(VERSION 3.10)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(lua-interpreter-demo)

# libraries

find_package(Lua 5.3 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})
message("lua Found: ${LUA_VERSION_STRING} inc: ${LUA_INCLUDE_DIR} lib: ${LUA_LIBRARIES}")

# compiler flags

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# cmake

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_VERBOSE_MAKEFILE ON)

# lib

add_library(lua_interpreter STATIC lua_interpreter.cxx)
set_target_properties(lua_interpreter PROPERTIES PUBLIC_HEADER lua_interpreter.hxx)
target_link_libraries(lua_interpreter ${LUA_LIBRARIES})

# demo exec

add_executable(demo_repl demo_repl.cxx)
target_link_libraries(demo_repl lua_interpreter)

enable_testing()
add_executable(demo_test demo_test.cxx)
target_link_libraries(demo_test lua_interpreter)
add_test(demo_test ${CMAKE_BINARY_DIR}/build/bin/demo_test)
