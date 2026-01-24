cmake_minimum_required(VERSION 3.15)
project(poker_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to MinSizeRel to save memory during compilation on small VMs
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
endif()

include(FetchContent)

# cpp-httplib (header-only HTTP server library)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# RapidJSON (v1.1.0)
FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG master
)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "Don't build RapidJSON docs")
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Don't build RapidJSON examples")
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "Don't build RapidJSON tests")
FetchContent_MakeAvailable(rapidjson)

# Source files organized by module
set(SOURCES
    main.cpp
    core/card.cpp
    core/deck.cpp
    evaluators/naive_evaluator.cpp
    engine/equity_engine.cpp
    engine/shared_memory_writer.cpp
    api/server.cpp
    api/job_manager.cpp
    api/json_utils.cpp
)

add_executable(poker_engine ${SOURCES})

target_include_directories(poker_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${httplib_SOURCE_DIR}
    ${rapidjson_SOURCE_DIR}/include
)

target_link_libraries(poker_engine PRIVATE
    pthread
    # rt # Not needed on macOS usually, but good for Linux. 
)

# On Linux, link rt for shared memory
if(UNIX AND NOT APPLE)
    target_link_libraries(poker_engine PRIVATE rt)
endif()

install(TARGETS poker_engine DESTINATION bin)

# --- Testing ---

# GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(unit_tests
    tests/test_hand_types.cpp
    tests/test_json_utils.cpp
    tests/test_cactus_kev.cpp
    tests/test_ph_evaluator.cpp
    tests/test_two_plus_two.cpp
    tests/test_omp_eval.cpp
    evaluators/cactus_kev_evaluator.cpp
    evaluators/ph_evaluator.cpp
    evaluators/two_plus_two_evaluator.cpp
    evaluators/omp_eval.cpp
    api/json_utils.cpp
    core/card.cpp
    engine/shared_memory_writer.cpp
    api/job_manager.cpp
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${httplib_SOURCE_DIR}
    ${rapidjson_SOURCE_DIR}/include
)

target_link_libraries(unit_tests PRIVATE
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
