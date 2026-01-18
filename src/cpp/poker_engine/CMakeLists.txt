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
    GIT_TAG v1.1.0
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
