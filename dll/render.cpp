/*
 * AC Skin Changer - Render Module
 * Handles DX11 render target lifecycle.
 */

#include "core.h"

namespace Render {
    void Initialize() {
        LogMsg("Render module initialized");
    }

    void Shutdown() {
        if (G::pRenderTargetView) {
            G::pRenderTargetView->Release();
            G::pRenderTargetView = nullptr;
        }
        LogMsg("Render module shutdown");
    }
}
