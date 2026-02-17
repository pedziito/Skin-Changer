/**
 * ACE Engine — Master Include
 *
 * A single include to access the entire ACE UI Framework.
 * This is the new modular engine built from the ground up.
 *
 * Architecture:
 *   core/      — Types, memory, reflection, events, thread pool
 *   render/    — GPU-agnostic backend, DX11, font atlas
 *   input/     — Platform-agnostic input system
 *   ui/        — Widget system, theming, layout, animation, dock system
 *   ui/widgets — Core widgets, property inspector, node editor, graph editor, viewport
 *   scripting/ — Lua/Python script engine
 *   hot_reload — File watching, DLL hot-reload, asset management
 */

#pragma once

// === Core ===
#include "core/types.h"
#include "core/memory.h"
#include "core/reflection.h"
#include "core/event.h"
#include "core/thread_pool.h"

// === Render ===
#include "render/render_backend.h"
#include "render/dx11_backend.h"
#include "render/font_atlas.h"

// === Input ===
#include "input/input_system.h"

// === UI System ===
#include "ui/widget.h"
#include "ui/theme.h"
#include "ui/animation.h"
#include "ui/layout.h"
#include "ui/ui_context.h"
#include "ui/dock_system.h"

// === Widgets ===
#include "ui/widgets/widgets.h"
#include "ui/widgets/property_inspector.h"
#include "ui/widgets/node_editor.h"
#include "ui/widgets/graph_editor.h"
#include "ui/widgets/viewport.h"

// === Scripting ===
#include "scripting/script_engine.h"

// === Hot Reload ===
#include "hot_reload/hot_reload.h"

// ============================================================================
// VERSION
// ============================================================================
#define ACE_ENGINE_VERSION_MAJOR 2
#define ACE_ENGINE_VERSION_MINOR 0
#define ACE_ENGINE_VERSION_PATCH 0
#define ACE_ENGINE_VERSION_STRING "2.0.0"

namespace ace {

inline constexpr struct {
    int major = ACE_ENGINE_VERSION_MAJOR;
    int minor = ACE_ENGINE_VERSION_MINOR;
    int patch = ACE_ENGINE_VERSION_PATCH;
    const char* string = ACE_ENGINE_VERSION_STRING;
} EngineVersion;

} // namespace ace
