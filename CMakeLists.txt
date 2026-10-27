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

add_executable(leduc_test tests/leduc_test.cpp)
target_link_libraries(leduc_test PRIVATE full_poker)
add_test(NAME leduc_test COMMAND leduc_test)

add_executable(reservoir_buffer_test tests/reservoir_buffer_test.cpp)
target_link_libraries(reservoir_buffer_test PRIVATE full_poker)
add_test(NAME reservoir_buffer_test COMMAND reservoir_buffer_test)

add_executable(kuhn_bench bench/kuhn_bench.cpp)
target_link_libraries(kuhn_bench PRIVATE full_poker)

add_executable(leduc_bench bench/leduc_bench.cpp)
target_link_libraries(leduc_bench PRIVATE full_poker)

find_package(Torch QUIET)

if(Torch_FOUND)
	add_executable(torch_smoke_test tests/torch_smoke_test.cpp)
	target_link_libraries(torch_smoke_test PRIVATE full_poker ${TORCH_LIBRARIES})

	add_executable(advantage_network_test tests/advantage_network_test.cpp)
	target_link_libraries(advantage_network_test PRIVATE full_poker ${TORCH_LIBRARIES})
	add_test(NAME advantage_network_test COMMAND advantage_network_test)

	add_executable(deep_cfr_test tests/deep_cfr_test.cpp)
	target_link_libraries(deep_cfr_test PRIVATE full_poker ${TORCH_LIBRARIES})
	add_test(NAME deep_cfr_test COMMAND deep_cfr_test)
endif()