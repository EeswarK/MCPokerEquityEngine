cmake_minimum_required(VERSION 3.15)
project(telemetry_collector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(IXWEBSOCKET REQUIRED ixwebsocket)

find_package(Flatbuffers REQUIRED)

set(FLATBUFFERS_FLATC_EXECUTABLE flatc)
set(TELEMETRY_FBS ${CMAKE_CURRENT_SOURCE_DIR}/telemetry.fbs)
set(TELEMETRY_GENERATED ${CMAKE_CURRENT_BINARY_DIR}/telemetry_generated.h)

add_custom_command(
    OUTPUT ${TELEMETRY_GENERATED}
    COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE} -c -o ${CMAKE_CURRENT_BINARY_DIR} ${TELEMETRY_FBS}
    DEPENDS ${TELEMETRY_FBS}
    COMMENT "Generating FlatBuffers headers"
)

set(SOURCES
    main.cpp
    shared_memory.cpp
    metrics_collector.cpp
    websocket_client.cpp
)

set(HEADERS
    shared_memory.h
    metrics_collector.h
    websocket_client.h
)

add_executable(telemetry_collector ${SOURCES} ${HEADERS})

target_include_directories(telemetry_collector PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${IXWEBSOCKET_INCLUDE_DIRS}
)

target_link_libraries(telemetry_collector
    pthread
    ${IXWEBSOCKET_LIBRARIES}
    rt
)

install(TARGETS telemetry_collector DESTINATION bin)
