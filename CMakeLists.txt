cmake_minimum_required(VERSION 3.15)
project(CS2SkinChanger VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# SKIN CHANGER DLL (injected into cs2.exe)
# Custom ACE rendering engine — zero ImGui dependency
# ============================================================================
add_library(skin_changer SHARED
    dll/dllmain.cpp
    dll/hooks.cpp
    dll/offsets.cpp
    dll/netvars.cpp
    dll/inventory.cpp
    dll/menu.cpp
    dll/config.cpp
    dll/render.cpp
    dll/skindb.cpp
    engine/ace_renderer.cpp
    engine/ace_ui.cpp
)

target_include_directories(skin_changer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dll
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/json
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

target_link_libraries(skin_changer PRIVATE
    d3d11
    dxgi
    dxguid
    d3dcompiler
    dwmapi
    wininet
    shell32
    user32
    gdi32
)

# Enable SEH for __try/__except in inventory.cpp
if(MSVC)
    target_compile_options(skin_changer PRIVATE /EHa)
else()
    target_compile_definitions(skin_changer PRIVATE _WIN32_WINNT=0x0A00)
endif()

set_target_properties(skin_changer PROPERTIES
    PREFIX ""
    OUTPUT_NAME "skin_changer"
    SUFFIX ".dll"
)

# ============================================================================
# LOADER EXECUTABLE (injects DLL into cs2.exe)
# ============================================================================
add_executable(ac_loader
    loader/loader.cpp
)

target_link_libraries(ac_loader PRIVATE
    advapi32
    shell32
    user32
)

set_target_properties(ac_loader PROPERTIES
    WIN32_EXECUTABLE FALSE
    OUTPUT_NAME "AC_Loader"
)

# ============================================================================
# ADMIN CLI TOOL (optional)
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/admin/main.cpp")
    add_executable(cs2admin
        admin/main.cpp
    )
    target_include_directories(cs2admin PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/admin
    )
    message(STATUS "Admin tool: cs2admin.exe (enabled)")
endif()

# ============================================================================
# LICENSE GENERATOR GUI (standalone DX11 app, same ACE theme)
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/admin/license_gui.cpp")
    add_executable(ac_license_gen WIN32
        admin/license_gui.cpp
        engine/ace_renderer.cpp
        engine/ace_ui.cpp
    )
    target_include_directories(ac_license_gen PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor
    )
    target_link_libraries(ac_license_gen PRIVATE
        d3d11
        dxgi
        d3dcompiler
        user32
        gdi32
        shell32
    )
    set_target_properties(ac_license_gen PROPERTIES
        OUTPUT_NAME "AC_LicenseGen"
    )
    message(STATUS "License GUI: AC_LicenseGen.exe (enabled)")
endif()

# ============================================================================
# Copy config files to output
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/skins.json")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/config/skins.json
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/skins.json
        COPYONLY
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/offsets.json")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/config/offsets.json
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/offsets.json
        COPYONLY
    )
endif()

# ============================================================================
# Build Summary
# ============================================================================
message(STATUS "")
message(STATUS "=== CS2 Skin Changer v3.0.0 Build ===")
message(STATUS "  DLL:      skin_changer.dll (injected into cs2.exe)")
message(STATUS "  Loader:   AC_Loader.exe (DLL injector)")
message(STATUS "  License:  AC_LicenseGen.exe (admin GUI)")
message(STATUS "  Engine:   ACE Custom Rendering Engine (zero ImGui)")
message(STATUS "  Shader: HLSL compiled at runtime via D3DCompile")
message(STATUS "  Font:   stb_truetype (system TTF loading)")
message(STATUS "  Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "======================================")
message(STATUS "")

