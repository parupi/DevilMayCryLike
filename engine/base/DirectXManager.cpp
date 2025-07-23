#include "Windows.h"

#include "base/DirectXManager.h"
#include <cassert>
#include <format>
#include <base/SrvManager.h>
#include <dxgi1_6.h>
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"
#include <DirectXTex/d3dx12.h>

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

	InitializeCommand();
	CreateSwapChain();
	CreateDepthBuffer();
	CreateHeap();
	CreateRTVForOffScreen();
	CreateRenderTargetView();
	InitializeDepthStencilView();
	CreateFence();
	SetViewPort();
	SetScissor();
	InitializeDXCCompiler();
}

void DirectXManager::Finalize()
{
	// --- GPUの完了を確実に待つ処理 ---
	if (commandQueue_ && fence_) {
		fenceValue_++;
		HRESULT hr = commandQueue_->Signal(fence_.Get(), fenceValue_);
		assert(SUCCEEDED(hr));

		if (fence_->GetCompletedValue() < fenceValue_) {
			if (!fenceEvent_) {
				fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			}
			hr = fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
			assert(SUCCEEDED(hr));
			WaitForSingleObject(fenceEvent_, INFINITE);
		}
	}

	// --- フェンスイベント解放 ---
	if (fenceEvent_) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}

	// --- リソース解放 ---
	commandList_.Reset();
	commandAllocator_.Reset();
	commandQueue_.Reset();

	swapChain_.Reset();

	for (auto& buffer : backBuffers_) {
		buffer.Reset();
	}
	backBuffers_.clear();

	offScreenResource_.Reset(); // ❗忘れがち

	depthBuffer_.Reset();

	rtvHeap_.Reset();
	dsvHeap_.Reset();

	fence_.Reset();

	graphicsDevice_.reset();

	dxcUtils_.Reset();
	dxcCompiler_.Reset();
	includeHandler_.Reset();

	Logger::Log("DirectXManager finalized.\n");
}


ComPtr<ID3D12DescriptorHeap> DirectXManager::CreateDescriptorHeap(ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
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

ComPtr<ID3D12Resource> DirectXManager::CreateDepthStencilTextureResource(ComPtr<ID3D12Device> device, int32_t width, int32_t height)
{
	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;											// Textureの幅
	resourceDesc.Height = height;										// Textureの長さ
	resourceDesc.MipLevels = 1;											// mipmapの数
	resourceDesc.DepthOrArraySize = 1;									// 奥行き or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;				// DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1;									// サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		// 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;		// DepthStencilとして使う通知

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;			// VRAM上に作る

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;					// 1.0f(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;		// フォーマット。Resourceと合わせる

	HRESULT hr;
	// Resourceの設定
	ComPtr<ID3D12Resource> resource = nullptr;
	hr = device->CreateCommittedResource(
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

void DirectXManager::CreateBufferResource(size_t sizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& outResource)
{
	// リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使う
	// リソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	// バッファリソース。テクスチャの場合はまた別の設定をする
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes; // リソースのサイズ
	// バッファの場合はこれらは1にする決まり
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// 実際にリソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = GetDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&outResource));
	assert(SUCCEEDED(hr));

	// 
#ifdef _DEBUG
	std::wstring debugName = L"Buffer_" + std::to_wstring(reinterpret_cast<uintptr_t>(outResource.Get()));
	outResource->SetName(debugName.c_str());
	OutputDebugStringW((L"Created " + debugName + L"\n").c_str());
#endif
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXManager::CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_CLEAR_VALUE color)
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

	// リソース生成
	ComPtr<ID3D12Resource> renderTexture;
	HRESULT hr = GetDevice()->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // 初期状態
		&color,
		IID_PPV_ARGS(&renderTexture)
	);

	if (FAILED(hr)) {
		// エラーハンドリング（ここでは簡易的にassert）
		assert(SUCCEEDED(hr) && "Failed to create RenderTextureResource.");
		return nullptr;
	}

	return renderTexture;
}

void DirectXManager::CreateRTVForOffScreen()
{
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	clearValue.Color[0] = 0.6f;
	clearValue.Color[1] = 0.5f;
	clearValue.Color[2] = 0.1f;
	clearValue.Color[3] = 1.0f;
	offScreenResource_ = CreateRenderTextureResource(WindowManager::kClientWidth, WindowManager::kClientHeight, clearValue.Format, clearValue);
}

void DirectXManager::CreateSRVForOffScreen(SrvManager* srvManager)
{
	srvIndex_ = srvManager->Allocate();
	srvHandle_.first = srvManager->GetCPUDescriptorHandle(srvIndex_);
	srvHandle_.second = srvManager->GetGPUDescriptorHandle(srvIndex_);
	srvManager->CreateSRVforTexture2D(srvIndex_, offScreenResource_.Get(), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1);
}

void DirectXManager::InitializeCommand()
{
	HRESULT hr;
	// コマンドキューを生成する
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = GetDevice()->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	assert(SUCCEEDED(hr));
	commandQueue_->SetName(L"CommandQueue");
	Logger::Log("Complete create ID3D12CommandQueue!!!\n");// コマンドキュー生成完了のログを出す

	// コマンドアロケータを生成する
	hr = GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	// コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));
	commandAllocator_->SetName(L"CommandAllocator");
	Logger::Log("Complete create ID3D12CommandAllocator!!!\n");// コマンドアロケータ生成完了のログを出す

	// コマンドリストを生成する
	hr = GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	// コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));
	commandList_->SetName(L"CommandList");
	Logger::Log("Complete create ID3D12GraphicsCommandList!!!\n");// コマンドリスト生成完了のログを出す
}

void DirectXManager::CreateSwapChain()
{
	HRESULT hr{};
	// スワップチェーンを生成する
	swapChainDesc.Width = WindowManager::kClientWidth; //画面の幅。ウィンドウンおクライアント領域を同じものにしておく
	swapChainDesc.Height = WindowManager::kClientHeight; //画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //色の形式
	swapChainDesc.SampleDesc.Count = 1; //マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2; //ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //モニタに移したら、中身を破棄
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = graphicsDevice_->GetFactory()->CreateSwapChainForHwnd(commandQueue_.Get(), winManager_->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	assert(SUCCEEDED(hr));
	Logger::Log("Complete create IDXGISwapChain4!!!\n");// スワップチェーン生成完了のログを出す
}

void DirectXManager::CreateDepthBuffer()
{
	// DepthStencilTextureをウィンドウサイズで作成
	depthBuffer_ = CreateDepthStencilTextureResource(GetDevice(), WindowManager::kClientWidth, WindowManager::kClientHeight);
	depthBuffer_->SetName(L"DepthBuffer");
}

void DirectXManager::CreateHeap()
{
	descriptorSizeRTV_ = GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV_ = GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// DescriptorHeapを生成
	rtvHeap_ = CreateDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, false);
	dsvHeap_ = CreateDescriptorHeap(GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	rtvHeap_->SetName(L"RTVHeap");
	dsvHeap_->SetName(L"DSVHeap");
}

void DirectXManager::CreateRenderTargetView()
{
	HRESULT hr{};
	// SwapChainからResourceを引っ張ってくる
	backBuffers_.resize(2);
	hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffers_[0]));
	// うまく取得できなければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&backBuffers_[1]));
	assert(SUCCEEDED(hr));
	Logger::Log("Complete get ID3D12Resource!!!\n");// リソースの取得完了のログを出す
	//backBuffers_[2] = offScreenResource_;
	// RTVの指定
	rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;			// 出力結果をSRGB二変換して書き込む
	rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;		// 2dテクスチャとして書き込む
	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

	rtvHandles_[0] = rtvStartHandle;
	GetDevice()->CreateRenderTargetView(backBuffers_[0].Get(), &rtvDesc_, rtvHandles_[0]);
	backBuffers_[0]->SetName(L"BackBuffer0");

	rtvHandles_[1].ptr = rtvHandles_[0].ptr + GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	GetDevice()->CreateRenderTargetView(backBuffers_[1].Get(), &rtvDesc_, rtvHandles_[1]);
	backBuffers_[1]->SetName(L"BackBuffer1");

	rtvHandles_[2].ptr = rtvHandles_[1].ptr + GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	GetDevice()->CreateRenderTargetView(offScreenResource_.Get(), &rtvDesc_, rtvHandles_[2]);
	offScreenResource_->SetName(L"OffScreenRenderTarget");
}

void DirectXManager::InitializeDepthStencilView()
{
	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;				// Format。基本的にはResourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;		// 2dTexture
	// DSVHeapの先頭にDSVを作る
	GetDevice()->CreateDepthStencilView(depthBuffer_.Get(), &dsvDesc, dsvHeap_->GetCPUDescriptorHandleForHeapStart());
	depthBuffer_->SetName(L"DepthStencilResource");
}

void DirectXManager::CreateFence()
{
	HRESULT hr{};
	hr = GetDevice()->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	// FenceのSignalを持つためのイベントを作成する
	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	fence_->SetName(L"Fence");
	assert(fenceEvent_ != nullptr);
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

void DirectXManager::BeginDraw()
{
	// バックバッファのインデックスを取得
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	TransitionResource(
		offScreenResource_.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);



	// バックバッファのリソースバリアを設定
	TransitionResource(
		backBuffers_[backBufferIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	// 描画ターゲットと深度ステンシルビューの設定
	SetRenderTargets(rtvHandles_[backBufferIndex]);

	// レンダーターゲットのクリア
	ClearRenderTarget(rtvHandles_[backBufferIndex]);

	// 深度ビューのクリア
	ClearDepthStencilView();

	// ビューポートとシザーレクトの設定
	SetViewportAndScissorRect();
}

void DirectXManager::BeginDrawForRenderTarget()
{
	TransitionResource(
		offScreenResource_.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	// 描画ターゲットと深度ステンシルビューの設定
	SetRenderTargets(rtvHandles_[2]);

	// レンダーターゲットのクリア
	ClearRenderTarget(rtvHandles_[2]);

	// 深度ビューのクリア
	ClearDepthStencilView();

	// ビューポートとシザーレクトの設定
	SetViewportAndScissorRect();
}

void DirectXManager::EndDraw()
{
	HRESULT hr{};
	UINT backBufferIndex;
	backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// FPS固定
	UpdateFixFPS();

	// 画面に描く処理はすべて終わり、画面に移すので、状態を遷移
	// 今回はRenderTargetからPresentにする
	barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	// TransitionBarrierを振る
	commandList_->ResourceBarrier(1, &barrier_);

	// コマンドリストの内容和確定させる。すべてのコマンドを詰んでからCloseすること
	hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	// GPUにコマンドリストの実行を行わせる
	ComPtr<ID3D12CommandList> commandLists[] = { commandList_ };
	commandQueue_->ExecuteCommandLists(1, commandLists->GetAddressOf());
	// GPUとOSに画面の交換を行うように通知する
	swapChain_->Present(1, 0);
	assert(SUCCEEDED(hr));

	// Fenceの値を更新
	fenceValue_++;
	// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
	commandQueue_->Signal(fence_.Get(), fenceValue_);

	if (fence_->GetCompletedValue() < fenceValue_) {
		// 指定したSignalにたどりつけてないので、たどり着くまで待つようにイベントを設定する
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		// イベントを待つ
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// 次のフレーム用のコマンドリストを準備
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

void DirectXManager::FlushUpload()
{
	HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	ID3D12CommandList* commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);

	// Fence シグナル & 待機
	fenceValue_++;
	commandQueue_->Signal(fence_.Get(), fenceValue_);
	if (fence_->GetCompletedValue() < fenceValue_) {
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// コマンドリストを次のフレーム用にリセット
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

void DirectXManager::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
	barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier_.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier_.Transition.pResource = resource;
	barrier_.Transition.StateBefore = stateBefore;
	barrier_.Transition.StateAfter = stateAfter;
	barrier_.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList_->ResourceBarrier(1, &barrier_);
}

void DirectXManager::SetRenderTargets(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

	commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
}

void DirectXManager::ClearDepthStencilView()
{
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void DirectXManager::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
	commandList_->ClearRenderTargetView(rtvHandle, clearValue.Color, 0, nullptr);
}

void DirectXManager::SetViewportAndScissorRect()
{
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);
}