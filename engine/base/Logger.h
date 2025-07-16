#pragma once
#include <string>

// ログ出力
namespace Logger
{
	// デバッグログ出力
	void Log(const std::string& message);

	// バッファリソース作成ログ出力
	void LogBufferCreation(const std::string& tag, void* resourcePtr, size_t sizeInBytes);

	// アサート失敗時のウィンドウ付きログ出力（汎用）
	void AssertWithMessage(bool condition, const char* expression, const char* message, const char* file, int line);
}

// マクロ：ファイル名、行番号を自動取得
#define ASSERT_MSG(expr, msg) Logger::AssertWithMessage((expr), #expr, msg, __FILE__, __LINE__)
