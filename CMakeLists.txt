cmake_minimum_required(VERSION 3.15)
project(CS2SkinChanger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# LOADER (cs2loader.exe) - The main entry point
# ============================================================================
add_executable(cs2loader
    loader/main.cpp
)

target_include_directories(cs2loader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/loader
)

set_target_properties(cs2loader PROPERTIES
    WIN32_EXECUTABLE FALSE
)

# ============================================================================
# INJECTOR DLL (CS2Changer.dll) - The game hook
# ============================================================================
file(GLOB_RECURSE INJECTOR_SOURCES 
    "injector/*.cpp"
)

file(GLOB_RECURSE INJECTOR_HEADERS 
    "injector/*.h"
)

add_library(CS2Changer SHARED
    ${INJECTOR_SOURCES}
    ${INJECTOR_HEADERS}
)

target_include_directories(CS2Changer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/injector
)

set_target_properties(CS2Changer PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
)

# ============================================================================
# CORE LIBRARY - Shared game manipulation logic
# ============================================================================
file(GLOB_RECURSE CORE_SOURCES 
    "src/*.cpp"
)

file(GLOB_RECURSE CORE_HEADERS 
    "include/*.h"
    "include/*.hpp"
)

# Create core library for both loader and DLL to use
add_library(CS2Core STATIC
    ${CORE_SOURCES}
)

target_include_directories(CS2Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(CS2Core PUBLIC
    comctl32
)

# Link core library to DLL and loader
target_link_libraries(CS2Changer PRIVATE
    CS2Core
)

target_link_libraries(cs2loader PRIVATE
    CS2Core
)

# ============================================================================
# Post-build: Copy DLL next to loader for easy distribution
# ============================================================================
add_custom_command(TARGET CS2Changer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:CS2Changer>
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/CS2Changer.dll
    COMMENT "Copying DLL to bin directory"
)

# Copy config files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/skins.json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/skins.json COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/offsets.json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/offsets.json COPYONLY)

# ============================================================================
# Summary
# ============================================================================
message(STATUS "CS2 Skin Changer Build Configuration")
message(STATUS "  Loader: cs2loader.exe")
message(STATUS "  DLL: CS2Changer.dll")
message(STATUS "  Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
