#pragma once
#ifdef _DEBUG

#include "Platform/WindowManager.h"
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <imgui/imgui.h>

class DirectXManager;

/// <summary>
/// エディタ用のゲームビュー。
///
/// ゲーム最終画を（バックバッファではなく）このクラスが持つオフスクリーンRTへ描き、
/// ImGui::Image でウィンドウの中に表示する。RTも表示サイズも kWidth x kHeight 固定で、
/// 拡縮は一切しない（1ドット=1ピクセル）。ウィンドウを小さくした場合はスクロールになる。
///
/// 使い方（1フレーム）:
///   Begin()      … RTへ切り替えてクリア
///   （ゲームの最終合成・UI・トランジションを描く）
///   End()        … SRVへ戻す
///   DrawWindow() … ImGuiのフレーム内で呼ぶ（Begin/Endより前のフレーム位置でよい）
/// </summary>
class EditorGameView
{
public:
	// ゲーム画面の描画解像度（＝表示サイズ）。エンジン側の設定をそのまま使う
	static constexpr uint32_t kWidth = WindowManager::kGameWidth;
	static constexpr uint32_t kHeight = WindowManager::kGameHeight;

	/// <param name="srvCpu">ImGuiのSRVヒープから確保済みのCPUハンドル</param>
	/// <param name="srvGpu">同じディスクリプタのGPUハンドル（ImTextureIDになる）</param>
	void Initialize(DirectXManager* dxManager,
		D3D12_CPU_DESCRIPTOR_HANDLE srvCpu, D3D12_GPU_DESCRIPTOR_HANDLE srvGpu);
	void Finalize();

	// ゲーム画面用RTへの描画を開始する
	void Begin();
	// 描画を終えてSRVとして読める状態に戻す
	void End();

	// ゲームビューウィンドウを構築する（ImGuiのNewFrame〜Renderの間で呼ぶ）
	void DrawWindow();

	ImTextureID GetTextureID() const { return static_cast<ImTextureID>(srvGpu_.ptr); }

	// ゲームビュー上にマウスカーソルが乗っているか
	bool IsHovered() const { return hovered_; }
	// ゲームビューウィンドウがフォーカスされているか
	bool IsFocused() const { return focused_; }

private:
	DirectXManager* dxManager_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> renderTarget_;
	uint32_t rtvIndex_ = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE srvCpu_{};
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpu_{};

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};

	// 今RTとして書き込める状態か。Begin/Endのバリアを二重に張らないために持つ
	bool isRenderTargetState_ = true;

	bool hovered_ = false;
	bool focused_ = false;
};

#endif // _DEBUG
