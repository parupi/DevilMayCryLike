#include "ImGuiManager.h"
#ifdef _DEBUG
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>
#include <dxgi.h>
#include <dxgi1_6.h> // DXGI 1.6まで必要な場合
#include <filesystem>

#include "Editor/Core/EditorHost.h"

namespace {
// ImGui用SRVヒープの枚数。
// 内訳はフォントアトラス（1.92では実行中に作り直されるので複数枚使うことがある）と、
// ImGui::Image で表示したいユーザーテクスチャ（ゲームビューなど）。
constexpr uint32_t kSrvHeapSize = 64;

// ImGuiのフォントをNoto Sans JPに差し替える。日本語グリフも読み込む。
// フォントファイルが見つからなければ何もしない（ImGuiの既定フォントが使われる）。
void LoadNotoSansJP()
{
	ImGuiIO& io = ImGui::GetIO();

	// 同梱のNoto Sans JPを優先。ImGui(stb_truetype)ではstaticのRegularが最も安定する。
	// 見つからなければ可変フォント→システムフォントの順にフォールバックする。
	const char* candidates[] = {
		"Font/Noto_Sans_JP/static/NotoSansJP-Regular.ttf",
		"Font/Noto_Sans_JP/NotoSansJP-VariableFont_wght.ttf",
		"C:/Windows/Fonts/NotoSansJP-VF.ttf",
	};

	for (const char* path : candidates) {
		if (!std::filesystem::exists(path)) {
			continue;
		}
		// 1.92以降はグリフを使う時に動的に焼くので、日本語の範囲指定はもう要らない
		io.Fonts->AddFontFromFileTTF(path, 18.0f);
		break;
	}
}
} // namespace


ImGuiManager& ImGuiManager::GetInstance()
{
	static ImGuiManager instance;
	return instance;
}

void ImGuiManager::Initialize(WindowManager* winManager, DirectXManager* directXManager)
{
	winManager_ = winManager;
	dxManager_ = directXManager;

	// ImGuiのコンテキストを生成
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	// ドッキング版。ウィンドウを画面端や他ウィンドウのタブへドッキングできるようにする
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// マルチビューポート（ImGuiウィンドウをOSウィンドウとして外へ出す）を使いたい場合は
	// 次の行を有効にする。Draw()側の UpdatePlatformWindows() も合わせて有効化すること。
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// フォントをNoto Sans JPに差し替える（DX12バックエンドがアトラスを作る前に登録する）
	LoadNotoSansJP();

	// ImGuiのスタイルを設定
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(winManager_->GetHwnd());

	// デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = kSrvHeapSize;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	// デスクリプタ―ヒープ生成
	HRESULT result = dxManager_->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap_));
	assert(SUCCEEDED(result));

	srvDescriptorSize_ = dxManager_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// 末尾から取り出すので、若いインデックスから使われるよう逆順に積んでおく
	srvFreeIndices_.reserve(kSrvHeapSize);
	for (uint32_t i = kSrvHeapSize; i > 0; --i) {
		srvFreeIndices_.push_back(i - 1);
	}

	ImGui_ImplDX12_InitInfo initInfo{};
	initInfo.Device = dxManager_->GetDevice();
	initInfo.CommandQueue = dxManager_->GetCommandContext()->GetCommandQueue();
	initInfo.NumFramesInFlight = static_cast<int>(dxManager_->GetBackBufferCount());
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
	initInfo.UserData = this;
	initInfo.SrvDescriptorHeap = srvHeap_.Get();
	initInfo.SrvDescriptorAllocFn = &ImGuiManager::SrvDescriptorAlloc;
	initInfo.SrvDescriptorFreeFn = &ImGuiManager::SrvDescriptorFree;
	ImGui_ImplDX12_Init(&initInfo);

	// ゲーム画面用オフスクリーン。SRVはImGuiのヒープ側に作る
	D3D12_CPU_DESCRIPTOR_HANDLE gameViewCpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE gameViewGpu{};
	bool allocated = AllocateSrvDescriptor(&gameViewCpu, &gameViewGpu);
	assert(allocated);
	(void)allocated;
	gameView_.Initialize(dxManager_, gameViewCpu, gameViewGpu);

	// エディタ本体の初期化。設定の読み込みとエンジン標準ウィンドウの登録はあちら側の仕事
	Editor::Initialize();
	// ゲーム画面のウィンドウ。オフスクリーンを持っているのがここなので、drawerとして預ける。
	// Editor::Initialize の外からの登録なので、エンジン側だと明示しておく
	Editor::AddWindowDrawer([this] { gameView_.DrawWindow(); }, EditorWindow::Origin::Engine);
}

bool ImGuiManager::AllocateSrvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
{
	if (srvFreeIndices_.empty()) {
		return false;
	}
	const uint32_t index = srvFreeIndices_.back();
	srvFreeIndices_.pop_back();

	*outCpu = srvHeap_->GetCPUDescriptorHandleForHeapStart();
	outCpu->ptr += static_cast<SIZE_T>(index) * srvDescriptorSize_;
	*outGpu = srvHeap_->GetGPUDescriptorHandleForHeapStart();
	outGpu->ptr += static_cast<UINT64>(index) * srvDescriptorSize_;
	return true;
}

void ImGuiManager::FreeSrvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
{
	const SIZE_T cpuBase = srvHeap_->GetCPUDescriptorHandleForHeapStart().ptr;
	const UINT64 gpuBase = srvHeap_->GetGPUDescriptorHandleForHeapStart().ptr;

	const uint32_t cpuIndex = static_cast<uint32_t>((cpu.ptr - cpuBase) / srvDescriptorSize_);
	const uint32_t gpuIndex = static_cast<uint32_t>((gpu.ptr - gpuBase) / srvDescriptorSize_);
	assert(cpuIndex == gpuIndex && cpuIndex < kSrvHeapSize);
	(void)gpuIndex;

	srvFreeIndices_.push_back(cpuIndex);
}

void ImGuiManager::SrvDescriptorAlloc(ImGui_ImplDX12_InitInfo* info,
	D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
{
	auto* self = static_cast<ImGuiManager*>(info->UserData);
	bool allocated = self->AllocateSrvDescriptor(outCpu, outGpu);
	// 足りなくなったら kSrvHeapSize を増やす
	assert(allocated);
	(void)allocated;
}

void ImGuiManager::SrvDescriptorFree(ImGui_ImplDX12_InitInfo* info,
	D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
{
	static_cast<ImGuiManager*>(info->UserData)->FreeSrvDescriptor(cpu, gpu);
}

void ImGuiManager::Begin()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// DockSpace・メニューバー・全ウィンドウはエディタ側が組み立てる。
	// このクラスはImGuiのDX12バックエンドの面倒だけを見る
	Editor::Draw();
}

void ImGuiManager::End()
{
	ImGui::Render();
}

void ImGuiManager::BeginGameViewRender()
{
	gameView_.Begin();
}

void ImGuiManager::EndGameViewRender()
{
	gameView_.End();
}

void ImGuiManager::Draw()
{
	ID3D12GraphicsCommandList* commandList = dxManager_->GetCommandList();

	// デスクリプタ―ヒープの配列をセットするコマンド
	ID3D12DescriptorHeap* ppHeaps[] = { srvHeap_.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	// 描画コマンド発行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	// マルチビューポートを有効にした場合はここでサブウィンドウを描く
	//if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
	//	ImGui::UpdatePlatformWindows();
	//	ImGui::RenderPlatformWindowsDefault();
	//}
}

void ImGuiManager::Finalize()
{
	// 設定の書き出しと登録済みdrawerの破棄。
	// drawerはthisをキャプチャしているので、ImGuiの破棄より先に手放しておく
	Editor::Finalize();

	gameView_.Finalize();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	srvHeap_.Reset(); // 明示的にリセット
	srvFreeIndices_.clear();
	dxManager_ = nullptr;
}

#endif // DEBUG
