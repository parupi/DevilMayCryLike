#include "Logger.h"
#include <dxgidebug.h>
#include <cassert>
#include <Windows.h>        // OutputDebugStringA
#include <sstream>          // std::stringstream
#include <string>           // std::string
#include <cstdint>          // uintptr_t

namespace Logger {
	void Log(const std::string& message)
	{
		OutputDebugStringA(message.c_str());
	}

    void LogBufferCreation(const std::string& tag, void* resourcePtr, size_t sizeInBytes)
    {
        std::stringstream ss;
        ss << "[" << tag << "] Buffer created. "
            << "Address: 0x" << std::hex << reinterpret_cast<uintptr_t>(resourcePtr)
            << ", Size: " << std::dec << sizeInBytes << " bytes.\n";

        OutputDebugStringA(ss.str().c_str());
    }

	void AssertWithMessage(bool condition, const char* expression, const char* message, const char* file, int line)
    {
        if (!condition) {
            char buffer[512];
            sprintf_s(buffer, "Assertion failed!\n\nExpression: %s\nMessage: %s\nFile: %s\nLine: %d",
                expression, message, file, line);

            // デバッグログに出力
            OutputDebugStringA(buffer);

            // ポップアップウィンドウを表示
            MessageBoxA(nullptr, buffer, "Assertion Failed", MB_OK | MB_ICONERROR);

            // アサートで停止（デバッガがあるときに止まる）
            assert(false);
        }
    }

    void CheckHR(HRESULT hr, const std::string& errorMessage)
    {
        if (FAILED(hr)) {
            Logger::Log(errorMessage);
            throw std::runtime_error(errorMessage);
        }
    }
}
