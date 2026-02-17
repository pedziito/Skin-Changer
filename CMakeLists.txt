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
file(GLOB CORE_SOURCES 
    "src/*.cpp"
)

# Exclude main.cpp from library (it's only for old builds)
list(FILTER CORE_SOURCES EXCLUDE REGEX "main\\.cpp$")

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
    ws2_32
    comctl32
)

# ============================================================================
# INJECTOR LIBRARY - Game hook compiled as static lib
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/injector")
    file(GLOB INJECTOR_SOURCES 
        "injector/*.cpp"
    )
    
    if(INJECTOR_SOURCES)
        add_library(CS2Injector STATIC
            ${INJECTOR_SOURCES}
        )
        
        target_include_directories(CS2Injector PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/injector
        )
        
        target_link_libraries(CS2Injector PRIVATE
            CS2Core
        )
        
        set(INJECTOR_LIBS CS2Injector)
    else()
        message(WARNING "No injector source files found, building without injector")
        set(INJECTOR_LIBS "")
    endif()
else()
    message(WARNING "Injector directory not found")
    set(INJECTOR_LIBS "")
endif()

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
    ${INJECTOR_LIBS}
)

set_target_properties(cs2inventory PROPERTIES
    WIN32_EXECUTABLE FALSE
)

# ============================================================================
# ADMIN CLI TOOL
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/admin/main.cpp")
    add_executable(cs2admin
        admin/main.cpp
    )
    
    target_include_directories(cs2admin PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/admin
    )
    
    message(STATUS "Admin tool: cs2admin.exe (enabled)")
else()
    message(WARNING "Admin tool source not found, skipping cs2admin.exe build")
endif()

# ============================================================================
# Copy config files
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/skins.json")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/skins.json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/skins.json COPYONLY)
    message(STATUS "Config: skins.json (copied)")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/offsets.json")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/offsets.json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/offsets.json COPYONLY)
    message(STATUS "Config: offsets.json (copied)")
endif()

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "CS2 Inventory Changer Build Configuration")
message(STATUS "  Executable: cs2inventory.exe (single-file)")
message(STATUS "  Admin Tool: cs2admin.exe")
message(STATUS "  Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

