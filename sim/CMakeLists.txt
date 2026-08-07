cmake_minimum_required(VERSION 3.15)
project(overby_power_propulsion_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(opp_physics INTERFACE)
target_include_directories(opp_physics INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(opp_sim_demo src/main.cpp)
target_link_libraries(opp_sim_demo PRIVATE opp_physics)

enable_testing()
add_executable(opp_sizing_tests tests/test_mhd_generator.cpp)
target_link_libraries(opp_sizing_tests PRIVATE opp_physics)
add_test(NAME opp_sizing_tests COMMAND opp_sizing_tests)
