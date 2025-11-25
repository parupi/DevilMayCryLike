#include "Windows.h"
#include "base/DirectXManager.h"
#include <cassert>
#include <format>
#include <dxgi1_6.h>
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"
#include <DirectXTex/d3dx12.h>
#include <math/function.h>

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

ComPtr<ID3D12Resource> DirectXManager::CreateDepthStencilTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format)
{
	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;											// Textureの幅
	resourceDesc.Height = height;										// Textureの長さ
	resourceDesc.MipLevels = 1;											// mipmapの数
	resourceDesc.DepthOrArraySize = 1;									// 奥行き or 配列Textureの配列数
	resourceDesc.Format = format;				// DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1;									// サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		// 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;		// DepthStencilとして使う通知

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;			// VRAM上に作る

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;					// 1.0f(最大値)でクリア
	depthClearValue.Format = format;		// フォーマット。Resourceと合わせる

	HRESULT hr;
	// Resourceの設定
	ComPtr<ID3D12Resource> resource = nullptr;
	hr = GetDevice()->CreateCommittedResource(
		&heapProperties,					// Heapの設定
		D3D12_HEAP_FLAG_NONE,				// Heapの特殊な設定
		&resourceDesc,						// Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	// 深度値を書き込む状態にしておく
		&depthClearValue,					// Clear最適地
		IID_PPV_ARGS(&resource)				// 作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));
	return resource;
}

ComPtr<ID3D12Resource> DirectXManager::CreateTextureResource(const DirectX::TexMetadata& metadata)
{
	// 1. metadataをもとにResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);									// Textureの幅
	resourceDesc.Height = UINT(metadata.height);								// textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);						// mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);					// 奥行 or 配列textureの配列数
	resourceDesc.Format = metadata.format;										// TextureのFormat
	resourceDesc.SampleDesc.Count = 1;											// サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);		// Textureの次元数。普段使っているものは2次元

	// 2. 利用するHeapの設定。非常に特殊な運用。02_04exで一般的なケース版がある
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.VisibleNodeMask = 1;
	heapProp.CreationNodeMask = 1;

	HRESULT hr;
	// 3. Resourceを生成する
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	hr = GetDevice()->CreateCommittedResource(
		&heapProp,							// Heapの設定
		D3D12_HEAP_FLAG_NONE,						// Heapの特殊な設定。特になし。
		&resourceDesc,								// Resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST,			// 初回のResourceState。Textureは基本読むだけ
		nullptr,									// Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource)
	);					// 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXManager::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
	CreateBufferResource(intermediateSize, intermediateResource);
	UpdateSubresources(commandList, texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
	// Textureへの転送後は利用可能とする、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

void DirectXManager::CreateBufferResource(size_t sizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& outResource, bool isUAV)
{
	// Heap 設定
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = isUAV ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;

	// Resource 設定
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = sizeInBytes;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = isUAV ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

	D3D12_RESOURCE_STATES initState = isUAV ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ;

	HRESULT hr = GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		initState,
		nullptr,
		IID_PPV_ARGS(&outResource)
	);
	assert(SUCCEEDED(hr));

#ifdef _DEBUG
	std::wstring debugName = L"Buffer_" + std::to_wstring(reinterpret_cast<uintptr_t>(outResource.Get()));
	outResource->SetName(debugName.c_str());
	OutputDebugStringW((L"Created " + debugName + L"\n").c_str());
#endif
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXManager::CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format)
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = width;                            // テクスチャの幅
	resourceDesc.Height = height;                          // テクスチャの高さ
	resourceDesc.DepthOrArraySize = 1;                     // テクスチャの深さ
	resourceDesc.MipLevels = 1;                            // Mipmapレベル
	resourceDesc.Format = format;                          // フォーマット指定
	resourceDesc.SampleDesc.Count = 1;                     // MSAA未使用
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // RenderTargetとして利用可能

	// Heapプロパティ
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;         // VRAM上に作成

	// クリアバリューを生成
	D3D12_CLEAR_VALUE clear = {};
	clear.Format = format;
	clear.Color[0] = r;
	clear.Color[1] = g;
	clear.Color[2] = b;
	clear.Color[3] = a;
	// リソース生成
	ComPtr<ID3D12Resource> renderTexture;
	HRESULT hr = GetDevice()->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // 初期状態
		&clear,
		IID_PPV_ARGS(&renderTexture)
	);

	if (FAILED(hr)) {
		// エラーハンドリング（ここでは簡易的にassert）
		assert(SUCCEEDED(hr) && "Failed to create RenderTextureResource.");
		return nullptr;
	}

	return renderTexture;
}

void DirectXManager::CreateDepthBuffer()
{
	depthBuffer_ = CreateDepthStencilTextureResource(WindowManager::kClientWidth, WindowManager::kClientHeight, DXGI_FORMAT_D24_UNORM_S8_UINT);
	depthBuffer_->SetName(L"DepthBuffer");

	// DSVManager側でheapを持つように変更したためここでは呼び出すだけ
	dsvManager_->Initialize(GetDevice());
	dsvManager_->CreateDsv(depthBuffer_.Get());
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

ComPtr<ID3D12Resource> DirectXManager::CreateGBufferResource(UINT width, UINT height, D3D12_CLEAR_VALUE clear)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Format = clear.Format;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	ComPtr<ID3D12Resource> resource;
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clear,
		IID_PPV_ARGS(&resource)
	);

	return resource;
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
	//// ビューポートとシザーレクトの設定
	//commandContext_->SetViewportAndScissor(viewport_, scissorRect_);
}

void DirectXManager::EndDraw()
{
	UINT backBufferIndex = swapChainManager_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = swapChainManager_->GetBackBuffer(backBufferIndex);

	// FPS固定
	UpdateFixFPS();

	// RenderTarget → Present
	commandContext_->TransitionResource(
		backBuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

		// FPS固定
	UpdateFixFPS();

	commandContext_->Flush();

	// GPUとOSに画面の交換を行うように通知する
	swapChainManager_->Present();

	commandContext_->Begin();
}

void DirectXManager::FlushUpload()
{
	commandContext_->FlushAndWait();
}
