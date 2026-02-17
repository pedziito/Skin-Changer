cmake_minimum_required(VERSION 3.15)
project(CS2InventoryChanger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

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

add_library(CS2Core STATIC
    ${CORE_SOURCES}
)

target_include_directories(CS2Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(CS2Core PUBLIC
    comctl32
)

# ============================================================================
# INJECTOR LIBRARY - Game hook compiled as static lib
# ============================================================================
file(GLOB_RECURSE INJECTOR_SOURCES 
    "injector/*.cpp"
)

file(GLOB_RECURSE INJECTOR_HEADERS 
    "injector/*.h"
)

add_library(CS2Injector STATIC
    ${INJECTOR_SOURCES}
    ${INJECTOR_HEADERS}
)

target_include_directories(CS2Injector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/injector
)

target_link_libraries(CS2Injector PRIVATE
    CS2Core
)

# ============================================================================
# SINGLE EXECUTABLE - Everything compiled in (no separate DLL)
# ============================================================================
add_executable(cs2inventory
    loader/main.cpp
)

target_include_directories(cs2inventory PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/loader
    ${CMAKE_CURRENT_SOURCE_DIR}/injector
)

target_link_libraries(cs2inventory PRIVATE
    CS2Core
    CS2Injector
)

set_target_properties(cs2inventory PROPERTIES
    WIN32_EXECUTABLE FALSE
)

# ============================================================================
# ADMIN CLI TOOL
# ============================================================================
add_executable(cs2admin
    admin/main.cpp
)

target_include_directories(cs2admin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/admin
)

# ============================================================================
# Copy config files
# ============================================================================
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/skins.json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/skins.json COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/offsets.json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/offsets.json COPYONLY)

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "CS2 Inventory Changer Build Configuration")
message(STATUS "  Executable: cs2inventory.exe (single-file)")
message(STATUS "  Admin Tool: cs2admin.exe")
message(STATUS "  Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

