/*
 * ACE Engine — Master Header
 * Fully custom rendering engine for CS2 overlay.
 * Zero ImGui dependency.
 *
 * Systems:
 *   1. DX11 Renderer — own vertex buffers, shader pipeline, draw command batching
 *   2. Font System   — stb_truetype TTF loading, alpha atlas with white pixel
 *   3. Input System  — WndProc handler, mouse/keyboard state tracking
 *   4. Animation     — smooth lerp, easing functions, pulse generators
 *   5. Component Tree — immediate-mode widgets: buttons, toggles, sliders, cards
 *   6. Layout Engine — cursor-based layout, child regions, scrolling
 *   7. Theme         — NEVERLOSE dark palette with accent colors
 */

#pragma once

#include "ace_renderer.h"
#include "ace_ui.h"

// ============================================================================
// GLOBAL ENGINE STATE
// ============================================================================
namespace ACE {
    // Core objects
    inline ACEFont       gFont;
    inline ACERenderer   gRenderer;
    inline ACEUIContext  gUI;

    // Initialize the engine after DX11 device is available
    inline bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
        // Initialize renderer (shaders, states, buffers)
        if (!gRenderer.Initialize(device, context))
            return false;

        // Load font — try system fonts in order
        const char* fontPaths[] = {
            "C:\\Windows\\Fonts\\segoeui.ttf",
            "C:\\Windows\\Fonts\\arial.ttf",
            "C:\\Windows\\Fonts\\tahoma.ttf",
            "C:\\Windows\\Fonts\\verdana.ttf",
        };

        bool fontLoaded = false;
        for (auto* path : fontPaths) {
            if (gFont.LoadFromFile(path, 14.0f)) {
                fontLoaded = true;
                break;
            }
        }

        if (!fontLoaded) return false;

        // Upload font atlas to GPU
        if (!gRenderer.CreateFontTexture(gFont))
            return false;

        // Wire font to draw lists
        gUI.drawList.font = &gFont;
        gUI.overlayDrawList.font = &gFont;

        return true;
    }

    // Begin a new frame
    inline void NewFrame(float displayW, float displayH, float deltaTime) {
        gUI.NewFrame(displayW, displayH, deltaTime);
    }

    // End frame and render
    inline void Render(ID3D11RenderTargetView* rtv, ID3D11DeviceContext* context) {
        gUI.EndFrame();

        // Set render target
        context->OMSetRenderTargets(1, &rtv, nullptr);

        // Render main draw list
        gRenderer.RenderDrawList(gUI.drawList, gUI.displayW, gUI.displayH);

        // Render overlay draw list (notifications, tooltips)
        if (!gUI.overlayDrawList.vtxBuffer.empty()) {
            gRenderer.RenderDrawList(gUI.overlayDrawList, gUI.displayW, gUI.displayH);
        }
    }

    // Shutdown and release resources
    inline void Shutdown() {
        gRenderer.Shutdown();
        gFont.Free();
    }
}
