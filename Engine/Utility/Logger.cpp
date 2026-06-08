#include "Logger.h"
#include <dxgidebug.h>
#include <cassert>
#include <Windows.h>        // OutputDebugStringA
#include <sstream>          // std::stringstream
#include <string>           // std::string
#include <cstdint>          // uintptr_t

std::string Logger::imguiLog_;

//namespace Logger {
void Logger::Log(const std::string& message)
{
	// デバッグ出力
	OutputDebugStringA(message.c_str());

	// ImGui 表示用ログへ追加
	imguiLog_ += message;

	// message に改行が無い可能性があるので補完
	if (!message.empty() && message.back() != '\n')
		imguiLog_ += "\n";
}

void Logger::LogBufferCreation(const std::string& tag, void* resourcePtr, size_t sizeInBytes)
{
	std::stringstream ss;
	ss << "[" << tag << "] Buffer created. "
		<< "Address: 0x" << std::hex << reinterpret_cast<uintptr_t>(resourcePtr)
		<< ", Size: " << std::dec << sizeInBytes << " bytes.\n";

	OutputDebugStringA(ss.str().c_str());
}

void Logger::AssertWithMessage(bool condition, const char* expression, const char* message, const char* file, int line)
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

void Logger::CheckHR(HRESULT hr, const std::string& errorMessage)
{
	if (FAILED(hr)) {
		Logger::Log(errorMessage);
		throw std::runtime_error(errorMessage);
	}
}
