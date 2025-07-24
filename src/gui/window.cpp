#include <gui/window.hpp>

#include "../../Resource.h"

#include <cassert>
#include <iostream>

using namespace Cass;

Window::Window(HINSTANCE hInstance, std::wstring className, std::wstring windowTitle, int width, int height)
    : m_hInstance(hInstance)
    , m_className(className)
    , m_windowTitle(windowTitle)
    , m_width(width)
    , m_height(height)
    , m_hWnd(0) { }

bool Window::Initialize(std::unique_ptr<WindowEventHandler> _eventManager) {
    assert(_eventManager != nullptr);

    eventManager = std::move(_eventManager);

    if (RegisterWindowClass() == 0) {
        std::cout << "Failed to reigster the window class\n";
        return false;
    }

    m_hWnd = CreateWindowW(m_className.c_str(), m_windowTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height, NULL, NULL, m_hInstance, eventManager.get());
    if (m_hWnd == NULL) {
        std::cout << "Failed to create the main window\n";
        return false;
    }

    return true;
}

// ---------- Protected methods

LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    WindowEventHandler* eventManager = nullptr;
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        eventManager = reinterpret_cast<WindowEventHandler*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(eventManager));
    }
    else {
        eventManager = reinterpret_cast<WindowEventHandler*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
    
    if (eventManager == nullptr) {
        // Class was not assigned during window creation or is invalidated, use default window Proc
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    // Handle the message

    switch (message) {
    case WM_PAINT:
        eventManager->OnPaint();
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

// ---------- Private methods

ATOM Window::RegisterWindowClass() {
    WNDCLASSEXW wcex;
    ZeroMemory(&wcex, sizeof(WNDCLASSEX));

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Window::WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = m_hInstance;
    wcex.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_CASS));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = m_className.c_str();
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}