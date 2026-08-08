#pragma once
#include <Windows.h>
#include <string>
class WindowManager
{
public:
	// クライアント領域（＝OSウィンドウ／バックバッファ）のサイズ
	static const uint32_t kClientWidth = 1920;
	static const uint32_t kClientHeight = 1080;

	// ゲーム画面の描画解像度。UIスプライトの座標系もこれ。
	// ウィンドウサイズとは独立させてある：
	//   Debug   … この解像度のまま ImGui の Game ウィンドウへ等倍で表示する
	//   Release … 最終合成でバックバッファ(kClientWidth x kClientHeight)へ引き伸ばす
	// UIは1280x720前提の座標で組まれているので、ここを変えるとUIの配置が崩れる
	static const uint32_t kGameWidth = 1280;
	static const uint32_t kGameHeight = 720;

public: // 静的メンバ変数

	/// <summary>
	/// ウィンドウプロシージャ
	/// </summary>
	/// <param name="hwnd">ウィンドウハンドル</param>
	/// <param name="msg">メッセージ番号</param>
	/// <param name="wparam">メッセージ情報1</param>
	/// <param name="lparam">メッセージ情報2</param>
	/// <returns>成否</returns>
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public: // メンバ変数
	// 初期化
	void Initialize();
	// メッセージの処理
	bool ProcessMessage();
	// 終了
	void Finalize();

	/// <summary>
	/// 通常ウィンドウ ⇔ 全画面（ボーダーレス）を切り替える。F11で呼ばれる。
	///
	/// 枠を外してモニタいっぱいに広げるだけで、ディスプレイモードは変更しない。
	/// クライアント領域がモニタと同じ大きさになるので、モニタが1920x1080なら
	/// バックバッファ(kClientWidth x kClientHeight)と一致して等倍で出る。
	/// それ以外の解像度のモニタではスワップチェインが引き伸ばされる。
	/// </summary>
	void ToggleFullscreen();
	bool IsFullscreen() const { return isFullscreen_; }

	/// <summary>
	/// アプリの終了を要求する。タイトルの QUIT など、ゲーム側から抜けたいときに呼ぶ。
	///
	/// 次の ProcessMessage() が true を返すようになり、メインループが素直に抜けて
	/// 通常の Finalize を通る。メッセージキューに積まずに自前のフラグで見ているのは、
	/// PeekMessage が1フレームに1件しか捌かないため、WM_QUIT の到着が遅れうるから
	/// </summary>
	static void RequestQuit() { quitRequested_ = true; }
	static bool IsQuitRequested() { return quitRequested_; }

	// getter
	HWND GetHwnd() const { return hwnd_; }
	HINSTANCE GetHInstance() const { return wndClass_.hInstance; }
private:
	// Window関連
	HWND hwnd_ = nullptr;   // ウィンドウハンドル
	WNDCLASS wndClass_{}; // ウィンドウクラス
	MSG msg_{};

	// 全画面中か
	bool isFullscreen_ = false;
	// 全画面にする直前の位置とサイズ。戻すときに使う
	WINDOWPLACEMENT windowedPlacement_{ sizeof(WINDOWPLACEMENT) };

	// WindowProcはstaticなのでインスタンスへ辿るために持っておく。ウィンドウは1つだけ
	static inline WindowManager* instance_ = nullptr;

	// ゲーム側から終了を要求されたか
	static inline bool quitRequested_ = false;
};

