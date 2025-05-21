#include "Logger.h"
#include <dxgidebug.h>
#include <cassert>

namespace Logger {
	void Log(const std::string& message)
	{
		OutputDebugStringA(message.c_str());
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
}
