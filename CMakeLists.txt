cmake_minimum_required(VERSION 3.20)
project(full_poker CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(full_poker INTERFACE)
target_include_directories(full_poker INTERFACE src)

enable_testing()

add_executable(kuhn_test tests/kuhn_test.cpp)
target_link_libraries(kuhn_test PRIVATE full_poker)
add_test(NAME kuhn_test COMMAND kuhn_test)