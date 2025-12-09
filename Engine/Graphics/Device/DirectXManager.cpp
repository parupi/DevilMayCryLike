#include "Windows.h"
#include "DirectXManager.h"
#include <cassert>
#include <format>
#include <dxgi1_6.h>
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"
#include <DirectXTex/d3dx12.h>
#include <math/function.h>
#include "Graphics/Rendering/PSO/PSOManager.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;

void DirectXManager::Initialize(WindowManager* winManager)
{
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();

		// ID3D12Debug1 を取得して GPU ベース検証も有効化
		Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
		if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)))) {
			debugController1->SetEnableGPUBasedValidation(true);
			debugController1->SetEnableSynchronizedCommandQueueValidation(true);
		}
	}
#endif

	assert(winManager);

	winManager_ = winManager;

	// FPS固定初期化
	InitializeFixFPS();
	// デバイスの生成
	graphicsDevice_ = std::make_unique<GraphicsDevice>();
	// デバイスの初期化 (失敗したらエラーを出す)
	if (!graphicsDevice_->Initialize()) {
		Logger::Log("GraphicsDevice initialization failed.");
		throw std::runtime_error("Failed to initialize GraphicsDevice.");
	}

	// コマンドクラスの生成
	commandContext_ = std::make_unique<CommandContext>();
	// コマンドリストの初期化
	if (!commandContext_->Initialize(GetDevice())) {
		Logger::Log("CommandContext initialization failed.");
		throw std::runtime_error("Failed to initialize CommandContext.");
	}

	// スワップチェイン生成
	swapChainManager_ = std::make_unique<SwapChainManager>();
	// コマンドリストの初期化
	if (!swapChainManager_->Initialize(graphicsDevice_.get(), commandContext_.get(), winManager_)) {
		Logger::Log("SwapChain initialization failed.");
		throw std::runtime_error("Failed to initialize SwapChain.");
	}

	resourceFactory_ = std::make_unique<GpuResourceFactory>(GetDevice());

	resourceManager_ = std::make_unique<ResourceManager>();
	resourceManager_->Initialize(GetDevice());

	srvManager_ = std::make_unique<SrvManager>();
	srvManager_->Initialize(this);

	rtvManager_ = std::make_unique<RtvManager>();
	rtvManager_->Initialize(this);
	// RTVの生成
	CreateRenderTargetView();

	// dsvManagerの生成
	dsvManager_ = std::make_unique<DsvManager>();
	// dsvの生成
	CreateDepthBuffer();

	commandContext_->CreateFence();
	SetViewPort();
	SetScissor();
	InitializeDXCCompiler();
}

void DirectXManager::Finalize()
{
	// --- リソース解放 ---
	commandContext_.reset();

	swapChainManager_.reset();

	srvManager_.reset();
	rtvManager_.reset();
	dsvManager_.reset();

	depthBuffer_.Reset();

	resourceFactory_.reset();
	resourceManager_.reset();

	graphicsDevice_.reset();

	dxcUtils_.Reset();
	dxcCompiler_.Reset();
	includeHandler_.Reset();

	Logger::Log("DirectXManager finalized.\n");
}


ComPtr<ID3D12DescriptorHeap> DirectXManager::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to create descriptor heap.");
	}

	return descriptorHeap;
}

void DirectXManager::InitializeFixFPS()
{
	// 現在時刻を記録する
	reference_ = std::chrono::steady_clock::now();
}

void DirectXManager::UpdateFixFPS()
{
	// 1/60秒ぴったりの時間
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
	// 1/60秒よりわずかに短い時間
	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

	// 現在時間を取得する
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	// 前回記録からの経過時間を取得する
	std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	// 1/60秒 (よりわずかに短い時間) 立っていない場合
	if (elapsed < kMinTime || elapsed < kMinCheckTime) {
		// 1/60秒経過するまで微小なスリープを繰り返す
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			// 1マイクロ秒スリープ
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}
	// 現在の時間を記録する
	reference_ = std::chrono::steady_clock::now();
}

IDxcBlob* DirectXManager::CompileShader(const std::wstring& filePath, const wchar_t* profile)
{
	//1.hlslファイルを読む
	// これからシェーダーをコンパイルする旨をログに出す
	//Logger::Log(ConvertString(std::format(L"Begin compileShader,Path:{},profile{}\n", filePath, profile)));
	//hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	//読めなかったら止める
	assert(SUCCEEDED(hr));
	//読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;//UTF8の文字コードであることを通知

	//2.Compileする
	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E",L"main",						//エントリーポイントの指定。基本的にmain以外にはしない
		L"-T",profile,						//ShaderProfileの設定
		L"-Zi",L"-Qembed_debug",			//デバッグ用の情報を埋め込む
		L"-Od",								//最適化を外しておく
		L"-Zpr"								//メモリレイアウトは行優先
	};
	//実際にshaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler_->Compile(
		&shaderSourceBuffer,				//読み込んだファイル
		arguments,							//コンパイルオプション
		_countof(arguments),				//コンパイルオプションの数
		includeHandler_.Get(),						//includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)			//コンパイル結果
	);
	//コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));

	//3.警告・エラーが出ていないか確認する
	//警告・エラーが出てたらログに出して止める
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		Logger::Log(shaderError->GetStringPointer());
		//警告・エラーダメゼッタイ
		assert(false);
	}

	//4.Compile結果を受け取って返す
	//コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	//成功したログを出す
	//Logger::Log(ConvertString(std::format(L"Compile Succeeded,path:{},profile:{}\n", filePath, profile)));
	//もう使わないリソースを解放
	shaderSource->Release();
	shaderResult->Release();
	//実行用のバイナリを返却
	return shaderBlob;
}

[[nodiscard]]
ComPtr<ID3D12Resource> DirectXManager::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(
		GetDevice(),
		mipImages.GetImages(),
		mipImages.GetImageCount(),
		mipImages.GetMetadata(),
		subresources);

	uint64_t uploadBufferSize =
		GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));

	// ResourceManager から Upload Buffer を作る
	ComPtr<ID3D12Resource> uploadBuffer = resourceManager_->CreateUploadResource(uploadBufferSize);
	resourceManager_->AddPendingUpload(uploadBuffer);
	// --- データ転送 ---
	UpdateSubresources(
		GetCommandList(),
		texture,
		uploadBuffer.Get(),
		0, 0,
		UINT(subresources.size()),
		subresources.data());

	// --- Barrier 設定 ---
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;

	GetCommandList()->ResourceBarrier(1, &barrier);

	// UploadBuffer を返す（GPU が読み終わるまで生存させるため）
	return uploadBuffer;
}

void DirectXManager::RegisterResourceForRelease(Microsoft::WRL::ComPtr<ID3D12Resource> res)
{
	if (!res) return;
	// ここは pendingReleaseStaging_ に積む（Flush 時に fence と組にして移動する設計）
	pendingReleaseStaging_.push_back(std::move(res));

#ifdef _DEBUG
	// デバッグログ
	std::wstring nm;
	if (res) {
		// GetName は wchar_t* を受け取るので buffer 用意（最大 256 文字等）
		auto nameBuf = GetResourceDebugName(res.Get());
		nm = nameBuf;
	}
	OutputDebugStringW((L"RegisterResourceForRelease (staging): " + nm + L"\n").c_str());
#endif
}

void DirectXManager::OnBeginFrame()
{
	// currentFrameIndex_ は SwapChain の index 等で管理するのが楽
	int idxToClear = currentFrameIndex_; // これは例。実際は前フレーム index に合わせる
	// GPU の fence を待った後で clear すること（Begin() の fence 待ち後）
	pendingRelease_[idxToClear].clear(); // ComPtrs がここで解放される
}

void DirectXManager::MoveStagingToPending(uint64_t fenceValue)
{
	if (pendingReleaseStaging_.empty()) return;

	for (auto& res : pendingReleaseStaging_) {
		pendingReleases_.emplace_back(fenceValue, std::move(res));
	}
	pendingReleaseStaging_.clear();
}

void DirectXManager::ProcessPendingReleases()
{
	uint64_t completed = commandContext_->GetFence()->GetCompletedValue();

	OutputDebugStringA(("ProcessPendingReleases: completed fence=" + std::to_string(completed) + "\n").c_str());

	auto it = pendingReleases_.begin();
	while (it != pendingReleases_.end()) {
		uint64_t fenceTag = it->first;
#ifdef _DEBUG
		// log the resource name and address

		if (it->second) {
			auto nameBuf = GetResourceDebugName(it->second.Get());
			std::wstring wmsg = L"PendingRelease check: fenceTag=" + std::to_wstring(fenceTag) +
				L" name=" + std::wstring(nameBuf) +
				L"\n";
			OutputDebugStringW(wmsg.c_str());
		}
#endif
		if (fenceTag <= completed) {
			it = pendingReleases_.erase(it);
		} else {
			++it;
		}
	}
}

std::wstring DirectXManager::GetResourceDebugName(ID3D12Resource* resource)
{
	if (!resource) return L"(null)";

	UINT size = 0;
	HRESULT hr = resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, nullptr);

	if (hr != S_OK || size == 0) {
		return L"(no name)";
	}

	std::wstring name;
	name.resize(size / sizeof(wchar_t));

	hr = resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name.data());

	if (FAILED(hr)) {
		return L"(failed to get name)";
	}

	// null終端を削る
	if (!name.empty() && name.back() == L'\0') {
		name.pop_back();
	}

	return name;
}

void DirectXManager::CreateDepthBuffer()
{
	GpuResourceFactory::TextureDesc desc{};
	desc.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.usage = GpuResourceFactory::Usage::DepthStencil;

	depthBuffer_ = resourceFactory_->CreateTexture2D(desc);
	depthBuffer_->SetName(L"DepthBuffer");

	// DSVManager側でheapを持つように変更したためここでは呼び出すだけ
	dsvIndex_ = dsvManager_->Allocate();
	dsvManager_->Initialize(GetDevice());
	dsvManager_->CreateDsv(dsvIndex_, depthBuffer_.Get());
}

void DirectXManager::CreateRenderTargetView()
{
	// RTVディスクリプタの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// SwapChainManagerからバックバッファを取得してRTV作成
	size_t backBufferCount = swapChainManager_->GetBackBufferCount();
	for (UINT i = 0; i < backBufferCount; ++i) {
		ID3D12Resource* backBuffer = swapChainManager_->GetBackBuffer(i);

		// RTVハンドルはRTVManagerから index指定で取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvManager_->GetCPUDescriptorHandle(rtvManager_->Allocate());
		// RTVの生成
		GetDevice()->CreateRenderTargetView(backBuffer, &rtvDesc, rtvHandle);

#ifdef _DEBUG
		// リソースに名前をつける（デバッグ用）
		std::wstring name = L"BackBuffer" + std::to_wstring(i);
		backBuffer->SetName(name.c_str());
#endif
	}

	Logger::Log("Complete CreateRenderTargetViews!\n");
}

void DirectXManager::SetViewPort()
{
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport_.Width = WindowManager::kClientWidth;
	viewport_.Height = WindowManager::kClientHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

void DirectXManager::SetScissor()
{
	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect_.left = 0;
	scissorRect_.right = WindowManager::kClientWidth;
	scissorRect_.top = 0;
	scissorRect_.bottom = WindowManager::kClientHeight;
}

void DirectXManager::InitializeDXCCompiler()
{
	HRESULT hr{};
	// dxcCompilerを初期化
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils_.GetAddressOf()));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(dxcCompiler_.GetAddressOf()));
	assert(SUCCEEDED(hr));
	// 現時点でincludeはしないが、includeに対応するための設定を行っておく
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
}

void DirectXManager::SetMainRTV()
{
	auto cmd = GetCommandList();
	auto rtv = rtvManager_->GetCPUDescriptorHandle(0);
	cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
}

void DirectXManager::SetMainDepth(ID3D12DescriptorHeap* dsvHeap)
{
	auto cmd = GetCommandList();

	if (dsvHeap == nullptr) {
		// Depthなし
		cmd->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
		return;
	}

	auto dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	cmd->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
}

void DirectXManager::BeginDraw()
{
	// バックバッファのインデックスを取得
	UINT backBufferIndex = GetSwapChainManager()->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = GetSwapChainManager()->GetBackBuffer(backBufferIndex);
	auto* commandList = GetCommandList();

	// バックバッファのリソースバリアを設定
	commandContext_->TransitionResource(
		backBuffer,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	// 描画ターゲットの設定（バックバッファへ）
	commandContext_->SetRenderTarget(rtvManager_->GetCPUDescriptorHandle(backBufferIndex));

	float clearColor[4]{ r,g,b,a };
	commandContext_->ClearRenderTarget(rtvManager_->GetCPUDescriptorHandle(backBufferIndex), clearColor);

	// ビューポートとシザーレクトの設定
	commandContext_->SetViewportAndScissor(viewport_, scissorRect_);
}

void DirectXManager::Render(PSOManager* psoManager, uint32_t srvIndex)
{
	auto* commandList = GetCommandList();

	// ① まず DescriptorHeap をセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetHeap() };
	commandList->SetDescriptorHeaps(1, heaps);

	commandList->SetPipelineState(psoManager->GetFinalCompositePSO());
	commandList->SetGraphicsRootSignature(psoManager->GetFinalCompositeRootSignature());

	commandList->SetGraphicsRootDescriptorTable(0, srvManager_->GetGPUDescriptorHandle(srvIndex));

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void DirectXManager::EndDraw()
{
	UINT backBufferIndex = swapChainManager_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = swapChainManager_->GetBackBuffer(backBufferIndex);

	// RenderTarget → Present
	commandContext_->TransitionResource(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	// Flush は commandList->Close、Execute、Signal(fenceValue) をやる
	commandContext_->Flush();

	// CommandContext が Signal した fenceValue を取得して ResourceManager に通知
	uint64_t fenceValue = commandContext_->GetFenceValue();
	resourceManager_->OnFrameEnd(fenceValue);

	// Present
	swapChainManager_->Present();

	// Begin next frame (this waits for previous fence if necessary)
	commandContext_->Begin();

	// Process pending releases that are completed
	uint64_t completed = commandContext_->GetFence()->GetCompletedValue();
	resourceManager_->ProcessPendingReleases(completed);
	resourceManager_->ReleasePendingUploads();

	UpdateFixFPS();
}

void DirectXManager::FlushUpload()
{
	//commandContext_->FlushAndWait();
}
