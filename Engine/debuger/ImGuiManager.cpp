#include "ImGuiManager.h"
#ifdef _DEBUG
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>
#include <dxgi.h>
#include <dxgi1_6.h> // DXGI 1.6まで必要な場合

ImGuiManager* ImGuiManager::instance = nullptr;
std::once_flag ImGuiManager::initInstanceFlag;

ImGuiManager* ImGuiManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new ImGuiManager();
	});
	return instance;
}

void ImGuiManager::Initialize(WindowManager* winManager, DirectXManager* directXManager)
{
	winManager_ = winManager;
	dxManager_ = directXManager;

	// ImGuiのコンテキストを生成
	ImGui::CreateContext();

	//ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// ImGuiのスタイルを設定
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(winManager_->GetHwnd());

	// デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	// デスクリプタ―ヒープ生成
	HRESULT result = dxManager_->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap_));
	assert(SUCCEEDED(result));

	ImGui_ImplDX12_Init(
		dxManager_->GetDevice(),
		static_cast<int>(dxManager_->GetBackBufferCount()),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, srvHeap_.Get(),
		srvHeap_->GetCPUDescriptorHandleForHeapStart(),
		srvHeap_->GetGPUDescriptorHandleForHeapStart()
	);
}

void ImGuiManager::Begin()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("Debug Log"))
	{
		ImGui::TextUnformatted(Logger::GetImGuiLog().c_str());

		if (ImGui::Button("Clear"))
		{
			Logger::ClearImGuiLog();
		}
	}
	ImGui::End();
}

void ImGuiManager::End()
{
	ImGui::Render();
}

void ImGuiManager::Draw()
{
	ID3D12GraphicsCommandList* commandList = dxManager_->GetCommandList();

	// デスクリプタ―ヒープの配列をセットするコマンド
	ID3D12DescriptorHeap* ppHeaps[] = { srvHeap_.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	// 描画コマンド発行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void ImGuiManager::Finalize()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	srvHeap_.Reset(); // 明示的にリセット
	dxManager_ = nullptr;
	delete instance;
	instance = nullptr;
}

#endif // DEBUG