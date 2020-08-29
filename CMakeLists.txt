cmake_minimum_required(VERSION 3.10)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(lua-interpreter-demo)

# libraries

find_package(Lua 5.3)
include_directories(${LUA_INCLUDE_DIR})
message("lua Found: ${LUA_VERSION_STRING} inc: ${LUA_INCLUDE_DIR} lib: ${LUA_LIBRARIES}")

# compiler flags

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# cmake

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_VERBOSE_MAKEFILE ON)

# exec

add_library(lua_interpreter STATIC lua_interpreter.cxx)
target_link_libraries(lua_interpreter ${LUA_LIBRARIES})
add_executable(demo demo.cxx)
target_link_libraries(demo lua_interpreter)
