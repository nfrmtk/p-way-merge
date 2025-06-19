cmake_minimum_required(VERSION 3.20)
project(p-way-merge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)


file(GLOB_RECURSE PMERGE_SOURCES "${CMAKE_CURRENT_LIST_DIR}/pmerge/**.cpp")
file(GLOB_RECURSE PMERGE_TESTS "${CMAKE_CURRENT_LIST_DIR}/pmerge/*test.cpp")

list(REMOVE_ITEM PMERGE_SOURCES ${PMERGE_TESTS})

find_package(GTest REQUIRED)

add_library(pmerge ${PMERGE_SOURCES})
target_compile_options(pmerge PUBLIC "-mavx2")
target_compile_options(pmerge PUBLIC "-fsanitize=address,undefined")
target_link_options(pmerge PUBLIC "-fsanitize=address,undefined")

target_include_directories(pmerge PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
add_compile_options()
foreach (test ${PMERGE_TESTS})
    get_filename_component(filename ${test} NAME_WE)
    add_executable(${filename}_ut ${test})
    target_link_libraries(${filename}_ut pmerge)
    target_link_libraries(${filename}_ut GTest::gtest_main)
endforeach()