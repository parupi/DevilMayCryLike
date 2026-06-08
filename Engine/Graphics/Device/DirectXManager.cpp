#include "Windows.h"
#include "DirectXManager.h"
#include <cassert>
#include <format>
#include <dxgi1_6.h>
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"
#include <DirectXTex/d3dx12.h>
#include <Math/MathUtils.h>
#include "Graphics/Rendering/PSO/PSOManager.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;

void DirectXManager::Initialize(WindowManager* winManager) {
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();

		Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
		if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)))) {
			debugController1->SetEnableGPUBasedValidation(true);
			debugController1->SetEnableSynchronizedCommandQueueValidation(true);
		}
	}
#endif

	assert(winManager);
	winManager_ = winManager;

	frameTimer_ = std::make_unique<FrameTimer>();
	frameTimer_->Initialize();

	graphicsDevice_ = std::make_unique<GraphicsDevice>();
	if (!graphicsDevice_->Initialize()) {
		Logger::Log("GraphicsDevice initialization failed.");
		throw std::runtime_error("Failed to initialize GraphicsDevice.");
	}

	commandContext_ = std::make_unique<CommandContext>();
	if (!commandContext_->Initialize(GetDevice())) {
		Logger::Log("CommandContext initialization failed.");
		throw std::runtime_error("Failed to initialize CommandContext.");
	}

	swapChainManager_ = std::make_unique<SwapChainManager>();
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
	CreateRenderTargetView();

	dsvManager_ = std::make_unique<DsvManager>();
	CreateDepthBuffer();

	commandContext_->CreateFence();
	SetViewPort();
	SetScissor();

	shaderCompiler_ = std::make_unique<ShaderCompiler>();
	shaderCompiler_->Initialize();
}

void DirectXManager::Finalize() {
	commandContext_.reset();
	swapChainManager_.reset();

	srvManager_->Finalize();
	srvManager_.reset();
	rtvManager_->Finalize();
	rtvManager_.reset();
	dsvManager_->Finalize();
	dsvManager_.reset();

	depthBuffer_.Reset();

	resourceFactory_.reset();
	resourceManager_.reset();
	graphicsDevice_.reset();

	shaderCompiler_.reset();
	frameTimer_.reset();

	Logger::Log("DirectXManager finalized.\n");
}

ComPtr<ID3D12DescriptorHeap> DirectXManager::CreateDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
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

ComPtr<ID3D12Resource> DirectXManager::UploadTextureData(
	ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(
		GetDevice(),
		mipImages.GetImages(),
		mipImages.GetImageCount(),
		mipImages.GetMetadata(),
		subresources);

	uint64_t uploadBufferSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));

	ComPtr<ID3D12Resource> uploadBuffer = resourceManager_->CreateUploadResource(uploadBufferSize);
	resourceManager_->AddPendingUpload(uploadBuffer);

	UpdateSubresources(
		GetCommandList(),
		texture,
		uploadBuffer.Get(),
		0, 0,
		UINT(subresources.size()),
		subresources.data());

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	GetCommandList()->ResourceBarrier(1, &barrier);

	return uploadBuffer;
}

void DirectXManager::CreateDepthBuffer() {
	GpuResourceFactory::TextureDesc desc{};
	desc.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.usage = GpuResourceFactory::Usage::DepthStencil;

	depthBuffer_ = resourceFactory_->CreateTexture2D(desc);
	depthBuffer_->SetName(L"DepthBuffer");

	dsvIndex_ = dsvManager_->Allocate();
	dsvManager_->Initialize(GetDevice());
	dsvManager_->CreateDsv(dsvIndex_, depthBuffer_.Get());
}

void DirectXManager::CreateRenderTargetView() {
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	size_t backBufferCount = swapChainManager_->GetBackBufferCount();
	for (UINT i = 0; i < backBufferCount; ++i) {
		ID3D12Resource* backBuffer = swapChainManager_->GetBackBuffer(i);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvManager_->GetCPUDescriptorHandle(rtvManager_->Allocate());
		GetDevice()->CreateRenderTargetView(backBuffer, &rtvDesc, rtvHandle);

#ifdef _DEBUG
		std::wstring name = L"BackBuffer" + std::to_wstring(i);
		backBuffer->SetName(name.c_str());
#endif
	}
	Logger::Log("Complete CreateRenderTargetViews!\n");
}

void DirectXManager::SetViewPort() {
	viewport_.Width = WindowManager::kClientWidth;
	viewport_.Height = WindowManager::kClientHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

void DirectXManager::SetScissor() {
	scissorRect_.left = 0;
	scissorRect_.right = WindowManager::kClientWidth;
	scissorRect_.top = 0;
	scissorRect_.bottom = WindowManager::kClientHeight;
}

void DirectXManager::SetMainRTV() {
	auto cmd = GetCommandList();
	auto rtv = rtvManager_->GetCPUDescriptorHandle(0);
	cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
}

void DirectXManager::SetMainDepth(ID3D12DescriptorHeap* dsvHeap) {
	auto cmd = GetCommandList();
	if (dsvHeap == nullptr) {
		cmd->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
		return;
	}
	auto dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	cmd->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
}

void DirectXManager::BeginDraw() {
	UINT backBufferIndex = GetSwapChainManager()->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = GetSwapChainManager()->GetBackBuffer(backBufferIndex);

	commandContext_->TransitionResource(
		backBuffer,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	commandContext_->SetRenderTarget(rtvManager_->GetCPUDescriptorHandle(backBufferIndex));

	float clearColor[4]{r, g, b, a};
	commandContext_->ClearRenderTarget(rtvManager_->GetCPUDescriptorHandle(backBufferIndex), clearColor);
	commandContext_->SetViewportAndScissor(viewport_, scissorRect_);
}

void DirectXManager::Render(PSOManager* psoManager, uint32_t srvIndex) {
	auto* commandList = GetCommandList();

	ID3D12DescriptorHeap* heaps[] = {srvManager_->GetHeap()};
	commandList->SetDescriptorHeaps(1, heaps);

	commandList->SetPipelineState(psoManager->GetFinalCompositePSO());
	commandList->SetGraphicsRootSignature(psoManager->GetFinalCompositeRootSignature());
	commandList->SetGraphicsRootDescriptorTable(0, srvManager_->GetGPUDescriptorHandle(srvIndex));
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void DirectXManager::EndDraw() {
	UINT backBufferIndex = swapChainManager_->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = swapChainManager_->GetBackBuffer(backBufferIndex);

	commandContext_->TransitionResource(
		backBuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	commandContext_->Flush();

	uint64_t fenceValue = commandContext_->GetFenceValue();
	resourceManager_->OnFrameEnd(fenceValue);

	swapChainManager_->Present();

	commandContext_->Begin();

	uint64_t completed = commandContext_->GetFence()->GetCompletedValue();
	resourceManager_->ProcessPendingReleases(completed);
	resourceManager_->ReleasePendingUploads();

	frameTimer_->Update();
}
