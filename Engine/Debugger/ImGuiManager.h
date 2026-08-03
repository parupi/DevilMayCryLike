#pragma once
#ifdef _DEBUG

#include "Platform/WindowManager.h"
#include "Graphics/Device/DirectXManager.h"
#include "Editor/Core/EditorGameView.h"
#include <memory>
#include <mutex>
#include <vector>
class ImGuiManager
{
private:
	ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;
public:
	// シングルトンインスタンスの取得
	static ImGuiManager& GetInstance();
	// 初期化
	void Initialize(WindowManager* winManager, DirectXManager* directXManager);
	// 終了
	void Finalize();
	// ImGui受付開始
	void Begin();
	// ImGui受付終了
	void End();
	// 描画
	void Draw();

	// ゲーム画面用オフスクリーンへの描画開始／終了。RenderPipelineから呼ぶ
	void BeginGameViewRender();
	void EndGameViewRender();

	EditorGameView& GetGameView() { return gameView_; }

	// ImGui用SRVヒープからディスクリプタを1つ確保する。
	// ここで得たGPUハンドルの ptr をそのまま ImTextureID として ImGui::Image に渡せる
	bool AllocateSrvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu);
	void FreeSrvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu);

private:
	// ImGui 1.92 のフォントアトラスは実行中に作り直されるため、
	// バックエンドから何度でもディスクリプタを取り／返せるようにしておく必要がある
	static void SrvDescriptorAlloc(struct ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu);
	static void SrvDescriptorFree(struct ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu);

private:
	WindowManager* winManager_ = nullptr;
	DirectXManager* dxManager_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
	uint32_t srvDescriptorSize_ = 0;
	// 未使用ディスクリプタのインデックス（末尾から取り出す単純なフリーリスト）
	std::vector<uint32_t> srvFreeIndices_;

	EditorGameView gameView_;
};

#endif // DEBUG
