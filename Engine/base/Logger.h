#pragma once
#include <string>
#include <d3d12.h>
#ifdef _DEBUG
#include <imgui.h>
#endif // DEBUG

// マクロ：ファイル名、行番号を自動取得
#define ASSERT_MSG(expr, msg) Logger::AssertWithMessage((expr), #expr, msg, __FILE__, __LINE__)

class Logger
{
public:
	static void Log(const std::string& message);
	static void LogBufferCreation(const std::string& tag, void* resourcePtr, size_t sizeInBytes);
	static void AssertWithMessage(bool condition, const char* expression, const char* message, const char* file, int line);
	static void CheckHR(HRESULT hr, const std::string& errorMessage);

	// === ImGui ===
    // ImGui 側が読む用
    static const std::string& GetImGuiLog() { return imguiLog_; }

    // 必要に応じてクリアする用
    static void ClearImGuiLog() { imguiLog_.clear(); }

private:
	static std::string imguiLog_;
};
