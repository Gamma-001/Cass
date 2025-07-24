// Cass.cpp : Defines the entry point for the application.
//

#include "Cass.h"

#include "framework.h"
#include <d3d12/ResourceManager.hpp>
#include <gui/window.hpp>

#include <memory>

// Global Variables:
std::unique_ptr<Cass::D3d12ResourceManager> resourceManager;

// Event handler forward declaration
class MainWindowEventHandler : public Cass::WindowEventHandler {
public:
    void OnPaint() override;
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    Cass::Window mainWindow(hInstance, L"MainWindow", L"Cass", 800, 800);
    if (!mainWindow.Initialize(std::make_unique<MainWindowEventHandler>())) {
        return -1;
    }

    RECT rcWindow;
    GetClientRect(mainWindow.getHandle(), &rcWindow);

    resourceManager = std::make_unique<Cass::D3d12ResourceManager>(rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, mainWindow.getHandle());
    resourceManager->OnInit();

    ShowWindow(mainWindow.getHandle(), nCmdShow);
    UpdateWindow(mainWindow.getHandle());

    // Main message loop:
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CASS));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    resourceManager->OnDestroy();
    return (int) msg.wParam;
}

void MainWindowEventHandler::OnPaint() {
    if (resourceManager != nullptr) {
        resourceManager->OnUpdate();
        resourceManager->OnRender();
    }
}
