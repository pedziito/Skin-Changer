cmake_minimum_required(VERSION 3.15)
project(CS2SkinChanger VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static CRT linking — embeds VC++ runtime so users don't need redistributables
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

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

# Copy built DLL to build dir so the resource compiler can embed it
add_custom_command(TARGET skin_changer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:skin_changer>"
        "${CMAKE_CURRENT_BINARY_DIR}/skin_changer.dll"
    COMMENT "Copying DLL for embedding into loader..."
)

# Generate resource script that embeds the DLL binary + app icon
set(EMBED_DLL_PATH "${CMAKE_CURRENT_BINARY_DIR}/skin_changer.dll")
string(REPLACE "\\" "/" EMBED_DLL_PATH "${EMBED_DLL_PATH}")
set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/ac_logo.ico")
string(REPLACE "\\" "/" ICON_PATH "${ICON_PATH}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/loader_embed.rc"
    "101 RCDATA \"${EMBED_DLL_PATH}\"\n"
    "1 ICON \"${ICON_PATH}\"\n"
)

# ============================================================================
# LOADER EXECUTABLE (DX11 GUI app — subscription panel + injector)
# Built on ACE Engine v2.0 — zero ImGui dependency.
# ============================================================================
add_executable(ac_loader WIN32
    loader/loader.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/loader_embed.rc
)

target_include_directories(ac_loader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/loader
)

target_link_libraries(ac_loader PRIVATE
    ace_engine_v2
    advapi32
    shell32
)

add_dependencies(ac_loader skin_changer)

set_target_properties(ac_loader PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "AC_Loader"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(ac_loader PRIVATE /W4 /permissive- /Zc:preprocessor /EHa)
    # Require UAC elevation (admin) — uses built-in linker flag to avoid manifest merge conflicts
    target_link_options(ac_loader PRIVATE
        "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'"
    )
endif()

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

# Copy skin images to output directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/skins")
    add_custom_target(copy_skin_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/skins"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/skins"
        COMMENT "Copying skin images to build output..."
    )
endif()

# ============================================================================
# ACE ENGINE V2 — Full UI Framework (standalone editor demo)
# ============================================================================
set(ACE_ENGINE_SOURCES
    engine/render/dx11_backend.cpp
    engine/render/font_atlas.cpp
    engine/ui/widget.cpp
    engine/ui/ui_context.cpp
    engine/ui/dock_system.cpp
    engine/ui/widgets/widgets.cpp
    engine/ui/widgets/property_inspector.cpp
    engine/ui/widgets/node_editor.cpp
    engine/ui/widgets/graph_editor.cpp
    engine/ui/widgets/viewport.cpp
    engine/scripting/script_engine.cpp
    engine/hot_reload/hot_reload.cpp
)

# Static library so multiple targets can link
add_library(ace_engine_v2 STATIC ${ACE_ENGINE_SOURCES})

target_include_directories(ace_engine_v2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

set_target_properties(ace_engine_v2 PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(ace_engine_v2 PRIVATE /W4 /permissive-)
endif()

target_link_libraries(ace_engine_v2 PUBLIC
    d3d11
    dxgi
    user32
    gdi32
)

# Editor demo app
add_executable(ace_editor WIN32 app/main.cpp)
target_link_libraries(ace_editor PRIVATE ace_engine_v2)
set_target_properties(ace_editor PROPERTIES
    OUTPUT_NAME "ACE_Editor"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(ace_editor PRIVATE /W4 /permissive- /Zc:preprocessor)
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
message(STATUS "")
message(STATUS "=== ACE Engine v2.0 UI Framework ===")
message(STATUS "  Library:  ace_engine_v2 (static)")
message(STATUS "  Editor:   ACE_Editor.exe (demo app)")
message(STATUS "  C++20, DX11, Hybrid UI, Dock, Node Editor")
message(STATUS "  Scripting: Lua/Python (stub, link for full)")
message(STATUS "  Hot Reload: File watcher + DLL loader")
message(STATUS "======================================")
message(STATUS "")

