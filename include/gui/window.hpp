#pragma once

#include <string>
#include <memory>

#include <windows.h>

namespace Cass {
    class WindowEventHandler {
    public:
        virtual void OnPaint() = 0;
        virtual void OnSize(int newWidth, int newHeight) = 0;
    };

    class Window {
    public:
        Window(HINSTANCE hInstance, std::wstring className, std::wstring windowTitle, int width = 800, int height = 800);

        bool Initialize(std::unique_ptr<WindowEventHandler> _eventManager);

        inline const HWND getHandle() const { return m_hWnd; }

        std::unique_ptr<WindowEventHandler> eventManager;

    protected:
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        ATOM RegisterWindowClass();

        std::wstring    m_className;
        std::wstring    m_windowTitle;
        int             m_width;
        int             m_height;

        HINSTANCE   m_hInstance;
        HWND        m_hWnd;
    };
}