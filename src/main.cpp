#include "GUI.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GUI gui;
    
    if (!gui.Initialize(hInstance)) {
        MessageBoxW(nullptr, L"Failed to initialize GUI", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    return gui.Run();
}
