#include "LightingPath.h"
#include "GBufferManager.h"
#include "Graphics/Device/DirectXManager.h"
#include <offscreen/OffScreenManager.h>
#include <math/function.h>

LightingPath::~LightingPath()
{
	dxManager_ = nullptr;
	gBufferManager_ = nullptr;
	psoManager_ = nullptr;

	fullScreenVB_.Reset();
}

void LightingPath::Initialize(DirectXManager* dx, GBufferManager* gBuffer, PSOManager* psoManager)
{
	dxManager_ = dx;
	gBufferManager_ = gBuffer;
	psoManager_ = psoManager;

	CreateFullScreenVB();
	CreateGBufferSRVs();
	CreateOutputResource();
}

void LightingPath::CreateFullScreenVB()
{
	// フルスクリーンクアッド（左下原点の場合）
	FullScreenVertex vertices[3] = {
		{{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // 左上
		{{ 3.0f, 1.0f, 0.0f}, {2.0f, 0.0f}}, // 右上
		{{ -1.0f, -3.0f, 0.0f}, {0.0f, 2.0f}}, // 左下
	};

	const UINT vbSize = sizeof(vertices);
	
	// バッファを作成
	fullScreenVB_ = dxManager_->GetResourceManager()->CreateUploadBufferWithData(vertices, vbSize);

	// map
	FullScreenVertex* mapped = nullptr;
	fullScreenVB_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	memcpy(mapped, vertices, vbSize);
	fullScreenVB_->Unmap(0, nullptr);

	// VBV
	fullScreenVBV_.BufferLocation = fullScreenVB_->GetGPUVirtualAddress();
	fullScreenVBV_.SizeInBytes = vbSize;
	fullScreenVBV_.StrideInBytes = sizeof(FullScreenVertex);
}

void LightingPath::Begin()
{
	auto cmd = dxManager_->GetCommandList();
	auto* commandContext = dxManager_->GetCommandContext();

	// パイプラインのセット
	cmd->SetPipelineState(psoManager_->GetLightingPathPSO());
	cmd->SetGraphicsRootSignature(psoManager_->GetLightingPathSignature());

	// 出力リソースを RT に戻す
	commandContext->TransitionResource(
		outputRtvResource_.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	// 描画ターゲットの設定
	commandContext->SetRenderTarget(dxManager_->GetRtvManager()->GetCPUDescriptorHandle(outputRtvIndex_));

	// rtvのクリア（UI等で参照する場合は残す）
	float clearColor[4] = { 0.6f, 0.5f, 0.1f, 1.0f };
	commandContext->ClearRenderTarget(dxManager_->GetRtvManager()->GetCPUDescriptorHandle(outputRtvIndex_), clearColor);

	ID3D12DescriptorHeap* heaps[] = { dxManager_->GetSrvManager()->GetHeap() };
	dxManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// GBufferで作ったSRVをセットする
	cmd->SetGraphicsRootDescriptorTable(0, gBufferSrvTable_);
}

void LightingPath::End()
{
	auto cmd = dxManager_->GetCommandList();
	auto* commandContext = dxManager_->GetCommandContext();

	// FullScreenQuad 描画
	cmd->IASetVertexBuffers(0, 1, &fullScreenVBV_);
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);

	// 読み込み用に変更
	commandContext->TransitionResource(
		outputRtvResource_.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
}

void LightingPath::CreateGBufferSRVs()
{
	auto* srvManager = dxManager_->GetSrvManager();

	// ---- ① 4つ連続の枠を確保（初期化時に1度だけ） ----
	gBufferSrvStartIndex_ = srvManager->Allocate();   // t0
	uint32_t index1 = srvManager->Allocate();         // t1
	uint32_t index2 = srvManager->Allocate();         // t2
	uint32_t index3 = srvManager->Allocate();         // t3
	//dsvIndex_ = srvManager->Allocate();

	// ---- ② SRV生成（キャッシュ） ----
	srvManager->CreateSRVforTexture2D(
		gBufferSrvStartIndex_,
		gBufferManager_->GetResource(GBufferManager::GBufferType::Albedo),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		1
	);

	srvManager->CreateSRVforTexture2D(
		index1,
		gBufferManager_->GetResource(GBufferManager::GBufferType::Normal),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		1
	);

	srvManager->CreateSRVforTexture2D(
		index2,
		gBufferManager_->GetResource(GBufferManager::GBufferType::WorldPos),
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		1
	);

	srvManager->CreateSRVforTexture2D(
		index3,
		gBufferManager_->GetResource(GBufferManager::GBufferType::Material),
		DXGI_FORMAT_R8G8_UNORM,
		1
	);

	//srvManager->CreateSRVforTexture2D(
	//	dsvIndex_,
	//	gBufferManager_->GetResource(GBufferManager::GBufferType::Depth),
	//	DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
	//	1
	//);


	// ---- ③ GPUハンドルをキャッシュ ----
	gBufferSrvTable_ = srvManager->GetGPUDescriptorHandle(gBufferSrvStartIndex_);
}

void LightingPath::CreateOutputResource()
{
	auto* rtvManager = dxManager_->GetRtvManager();
	auto* srvManager = dxManager_->GetSrvManager();
	auto* commandContext = dxManager_->GetCommandContext();

	// リソースの生成
	GpuResourceFactory::TextureDesc desc;
	desc.clearColor[0] = 0.6f;
	desc.clearColor[1] = 0.5f;
	desc.clearColor[2] = 0.1f;
	desc.clearColor[3] = 1.0f;
	desc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.usage = GpuResourceFactory::Usage::RenderTarget;
	outputRtvResource_ = dxManager_->GetResourceFactory()->CreateTexture2D(desc);

	// rtvの生成
	outputRtvIndex_ = rtvManager->Allocate();
	dxManager_->GetRtvManager()->CreateRTV(outputRtvIndex_, outputRtvResource_.Get());

	commandContext->TransitionResource(
		outputRtvResource_.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	// SRV の生成（DirectXManager が最終合成で使えるようにする）
	outputSrvIndex_ = srvManager->Allocate();
	srvManager->CreateSRVforTexture2D(outputSrvIndex_, outputRtvResource_.Get(), desc.format, 1);
	outputSrvTable_ = srvManager->GetGPUDescriptorHandle(outputSrvIndex_);
}
