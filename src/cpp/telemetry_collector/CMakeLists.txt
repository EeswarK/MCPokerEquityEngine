cmake_minimum_required(VERSION 3.15)
project(telemetry_collector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Download and build FlatBuffers
set(FLATBUFFERS_BUILD_FLATC ON CACHE BOOL "Build flatc compiler" FORCE)
FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG v23.5.26
)
FetchContent_MakeAvailable(flatbuffers)

# Download and build IXWebSocket
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.3
)
set(USE_TLS OFF CACHE BOOL "Disable TLS for IXWebSocket")
FetchContent_MakeAvailable(ixwebsocket)

# Generate FlatBuffers code from schema
set(TELEMETRY_FBS ${CMAKE_CURRENT_SOURCE_DIR}/telemetry.fbs)
set(TELEMETRY_GENERATED_H ${CMAKE_CURRENT_BINARY_DIR}/telemetry_generated.h)

# Get flatc executable path - try different target names
if(TARGET flatc)
    set(FLATC_TARGET flatc)
elseif(TARGET flatbuffers::flatc)
    set(FLATC_TARGET flatbuffers::flatc)
else()
    message(FATAL_ERROR "flatc target not found. FlatBuffers may not have built flatc.")
endif()

set(FLATC_EXECUTABLE $<TARGET_FILE:${FLATC_TARGET}>)

add_custom_command(
    OUTPUT ${TELEMETRY_GENERATED_H}
    COMMAND ${FLATC_EXECUTABLE} -c -o ${CMAKE_CURRENT_BINARY_DIR} ${TELEMETRY_FBS}
    DEPENDS ${TELEMETRY_FBS} ${FLATC_TARGET}
    COMMENT "Generating FlatBuffers headers"
)

# Source files
set(SOURCES
    main.cpp
    shared_memory.cpp
    metrics_collector.cpp
    websocket_server.cpp
)

# Header files
set(HEADERS
    shared_memory.h
    metrics_collector.h
    websocket_server.h
)

# Create executable
add_executable(telemetry_collector ${SOURCES} ${HEADERS} ${TELEMETRY_GENERATED_H})

# Include directories
target_include_directories(telemetry_collector PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${flatbuffers_SOURCE_DIR}/include
    ${ixwebsocket_SOURCE_DIR}
)

# Link libraries
# Note: FlatBuffers is header-only, no linking needed
target_link_libraries(telemetry_collector PRIVATE
    pthread
    rt
    ixwebsocket
)

# Install
install(TARGETS telemetry_collector DESTINATION bin)
