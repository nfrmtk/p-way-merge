cmake_minimum_required(VERSION 3.20)
project(p-way-merge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)


file(GLOB_RECURSE PMERGE_SOURCES "${CMAKE_CURRENT_LIST_DIR}/pmerge/**.cpp")
file(GLOB_RECURSE PMERGE_TESTS "${CMAKE_CURRENT_LIST_DIR}/pmerge/*test.cpp")
set(PMERGE_BENCH "${CMAKE_CURRENT_LIST_DIR}/pmerge/tests/bench.cpp")
list(REMOVE_ITEM PMERGE_SOURCES ${PMERGE_TESTS})
list(REMOVE_ITEM PMERGE_SOURCES ${PMERGE_BENCH})

find_package(GTest REQUIRED)

add_library(pmerge ${PMERGE_SOURCES})
target_compile_options(pmerge PUBLIC "-mavx2")
target_compile_options(pmerge PUBLIC "-fuse-ld=lld")
target_compile_options(pmerge PUBLIC "-fno-omit-frame-pointer")
target_link_options(pmerge PUBLIC "-fuse-ld=lld")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")

    set(_SUGGEST_SANITIZE ON)
else()
    set(_SUGGEST_SANITIZE OFF)
endif()

option(PMERGE_SANITIZE _SUGGEST_SANITIZE)

if (PMERGE_SANITIZE)
message(status "choosing non-instrumented build")
    target_compile_options(pmerge PUBLIC "-fsanitize=address,undefined")
    target_link_options(pmerge PUBLIC "-fsanitize=address,undefined")
endif()
set(WORKING_TESTS "merge_test;pack_test;simd_two_way_vector_resource_test;loser_tree_test;spilling_block_test;ydb_interface_merge_test" LIST)

target_include_directories(pmerge PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

enable_testing()

add_custom_target(tests)

foreach (test ${PMERGE_TESTS})
    get_filename_component(filename ${test} NAME_WE)
    add_executable(${filename}_ut ${test})
    target_link_libraries(${filename}_ut pmerge)
    target_link_libraries(${filename}_ut GTest::gtest_main)
    if ("${filename}" IN_LIST WORKING_TESTS)
        gtest_discover_tests(${filename}_ut)
        add_dependencies(tests ${filename}_ut)
    endif()
endforeach()

add_executable(pmerge-benchmark "${CMAKE_CURRENT_SOURCE_DIR}/pmerge/tests/bench.cpp")

target_link_libraries(pmerge-benchmark PUBLIC pmerge)