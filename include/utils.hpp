#pragma once

#include <windows.h>

#include <exception>
#include <format>

namespace Cass {
    class ComException : public std::exception {
    private:
        HRESULT result;

    public:
        ComException(HRESULT hr) noexcept : result(hr) {}
        const char* what() const override;
    };

    inline void ThrowIfFailed(HRESULT hr) {
        if (FAILED(hr)) {
            char errorMessage[1024];
            FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                hr,
                0,
                errorMessage,
                0,
                nullptr
            );

            OutputDebugStringA(std::format("Cass::ComError::Code {:#x}\n", static_cast<uint32_t>(hr)).c_str());
            throw ComException(hr);
        }
    }
}