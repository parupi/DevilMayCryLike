#include "WindowManager.h"
#include "imgui.h"
#pragma comment(lib, "winmm.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lPatam);

// ウィンドウプロシ―ジャ
LRESULT CALLBACK WindowManager::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
	// メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		// ウィンドウが作成された
	case WM_CREATE:
		return 0;
		// ウィンドウが破棄された
	case WM_DESTROY:
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}
	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WindowManager::Initialize()
{
	// COMの初期化をする
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// ウィンドウプロシージャ
	wndClass_.lpfnWndProc = WindowProc;
	// ウィンドウクラス名(なんでもいい)
	wndClass_.lpszClassName = L"CG2WindowClass";
	// インスタンスハンドル
	wndClass_.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wndClass_.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスを登録する
	RegisterClass(&wndClass_);

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };

	// クライアント領域をもとに実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウの生成
	hwnd_ = CreateWindow(
		wndClass_.lpszClassName,			//利用するクラス名
		L"ゲームタイトル with GuchisEngin",						//タイトルバーの文字(なんでもいい)
		WS_OVERLAPPEDWINDOW,		//よく見るウィンドウスタイル
		CW_USEDEFAULT,				//表示X座標(WindowsOSに任せる)
		CW_USEDEFAULT,				//表示Y座標(WindowsOSに任せる)
		wrc.right - wrc.left,		//ウィンドウ横幅
		wrc.bottom - wrc.top,		//ウィンドウ縦幅
		nullptr,					//親ウィンドウハンドル
		nullptr,					//メニューハンドル
		wndClass_.hInstance,				//インスタンスハンドル
		nullptr						//オプション
	);

	// ウィンドウを表示する
	ShowWindow(hwnd_, SW_SHOW);

	// システムタイマーの制度を上げる
	timeBeginPeriod(1);
}

bool WindowManager::ProcessMessage()
{
	MSG msg{};
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	// ウィンドウの×ボタンが押されるまでループ
	if (msg.message == WM_QUIT) {
		return true;
	}
	return false;
}

void WindowManager::Finalize()
{
	CoUninitialize();
	CloseWindow(hwnd_);
}
