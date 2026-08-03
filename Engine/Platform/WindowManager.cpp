#include "WindowManager.h"
#ifdef _DEBUG
#include <imgui.h>
#endif // IMGUI
#include "Utility/Logger.h"
#pragma comment(lib, "winmm.lib")

#ifdef _DEBUG
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // IMGUI

// ウィンドウプロシージャ
LRESULT CALLBACK WindowManager::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
#ifdef _DEBUG
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
#endif // IMGUI

	// メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		// ウィンドウが作成された
	case WM_CREATE:
		return 0;
		// キーが押された
	case WM_KEYDOWN:
		// F11で全画面をトグルする。bit30は「前回も押されていたか」なのでオートリピートを弾く
		if (wparam == VK_F11 && (lparam & (1 << 30)) == 0) {
			if (instance_) {
				instance_->ToggleFullscreen();
			}
			return 0;
		}
		break;
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

	// staticなWindowProcから辿れるようにしておく
	instance_ = this;

	// ウィンドウプロシージャ
	wndClass_.lpfnWndProc = WindowProc;
	// ウィンドウクラス名(なんでもいい)
	wndClass_.lpszClassName = L"WindowClass";
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
		L"Eclipser with GuchisEngine",						//タイトルバーの文字(なんでもいい)
		WS_OVERLAPPEDWINDOW,		//よく見るウィンドウスタイル
		// クライアント1920x1080は枠を足すとデスクトップ(1920x1080)より大きい。
		// OS任せのカスケード配置だと右下が大きくはみ出すので、左上に固定して被害を減らす
		0,							//表示X座標
		0,							//表示Y座標
		wrc.right - wrc.left,		//ウィンドウ横幅
		wrc.bottom - wrc.top,		//ウィンドウ縦幅
		nullptr,					//親ウィンドウハンドル
		nullptr,					//メニューハンドル
		wndClass_.hInstance,				//インスタンスハンドル
		nullptr						//オプション
	);

	// ウィンドウを表示する
	ShowWindow(hwnd_, SW_SHOW);

	// システムタイマーの精度を上げる
	timeBeginPeriod(1);
}

void WindowManager::ToggleFullscreen()
{
	if (!hwnd_) return;

	const LONG style = GetWindowLong(hwnd_, GWL_STYLE);

	if (!isFullscreen_) {
		// 戻すときのために今の位置とサイズを覚えておく
		MONITORINFO monitorInfo{ sizeof(MONITORINFO) };
		if (!GetWindowPlacement(hwnd_, &windowedPlacement_) ||
			!GetMonitorInfo(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitorInfo)) {
			return;
		}

		// 枠とタイトルバーを外して、ウィンドウのあるモニタいっぱいに広げる
		SetWindowLong(hwnd_, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
		SetWindowPos(hwnd_, HWND_TOP,
			monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.top,
			monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		isFullscreen_ = true;
	} else {
		// 枠を戻して、覚えておいた位置とサイズに復帰する
		SetWindowLong(hwnd_, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(hwnd_, &windowedPlacement_);
		SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		isFullscreen_ = false;
	}
}

bool WindowManager::ProcessMessage()
{
	if (PeekMessage(&msg_, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg_);
		DispatchMessage(&msg_);
	}
	if (msg_.message == WM_QUIT) {
		return true;
	}
	return false;
}

void WindowManager::Finalize()
{
	if (hwnd_) {
		// DestroyWindowでWM_DESTROYが送られ、PostQuitMessageが呼ばれるのが理想
		DestroyWindow(hwnd_);
		hwnd_ = nullptr;
	}

	if (instance_ == this) {
		instance_ = nullptr;
	}

	// システムタイマー精度を元に戻す
	timeEndPeriod(1);

	// COMの終了
	CoUninitialize();

	Logger::Log("WindowManager finalized.\n");
}