#include "LightingPath.h"
#include "GBufferManager.h"
#include "base/DirectXManager.h"
#include <offscreen/OffScreenManager.h>

void LightingPath::Initialize(DirectXManager* dx, GBufferManager* gBuffer, PSOManager* psoManager)
{
	dxManager_ = dx;
	gBufferManager_ = gBuffer;
	psoManager_ = psoManager;

	CreateFullScreenVB();
	CreateGBufferSRVs();
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
	dxManager_->CreateBufferResource(vbSize, fullScreenVB_);

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

void LightingPath::Execute()
{
	auto* commandList = dxManager_->GetCommandList();
	auto* commandContext = dxManager_->GetCommandContext();

	dxManager_->SetMainRTV();
	dxManager_->SetMainDepth(nullptr);

	// シェーダに OffScreen の最終結果を SRV として渡す
	//commandList->SetGraphicsRootDescriptorTable(0, inputSrv_);

	// 残りのライト描画処理
	DrawDirectionalLight();
}

void LightingPath::Begin()
{
	// バックバッファのインデックスを取得
	UINT backBufferIndex = dxManager_->GetSwapChainManager()->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = dxManager_->GetSwapChainManager()->GetBackBuffer(backBufferIndex);
	auto cmd = dxManager_->GetCommandList();
	auto* commandContext = dxManager_->GetCommandContext();

	// バックバッファのリソースバリアを設定
	commandContext->TransitionResource(
		backBuffer,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxManager_->GetDsvManager()->GetDsvHandle();

	// 描画ターゲットと深度ステンシルビューの設定
	commandContext->SetRenderTarget(dxManager_->GetRtvManager()->GetCPUDescriptorHandle(backBufferIndex), dsvHandle);

	// レンダーターゲットのクリア
	float clearColor[4] = { 0.6f, 0.5f, 0.1f, 1.0f };
	clearColor[0] = powf(0.6f, 2.2f);
	clearColor[1] = powf(0.5f, 2.2f);
	clearColor[2] = powf(0.1f, 2.2f);

	commandContext->ClearRenderTarget(dxManager_->GetRtvManager()->GetCPUDescriptorHandle(backBufferIndex), clearColor);

	// 深度ビューのクリア
	commandContext->ClearDepth(dsvHandle);

	cmd->SetPipelineState(psoManager_->GetLightingPathPSO());
	cmd->SetGraphicsRootSignature(psoManager_->GetLightingPathSignature());

	// シェーダに OffScreen の最終結果を SRV として渡す
	cmd->SetGraphicsRootDescriptorTable(0, gBufferSrvTable_);
}

void LightingPath::DrawDirectionalLight()
{
	auto cmd = dxManager_->GetCommandList();

	// FullScreenQuad 描画
	cmd->IASetVertexBuffers(0, 1, &fullScreenVBV_);
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmd->DrawInstanced(3, 1, 0, 0);
}

void LightingPath::End()
{
	//UINT backBufferIndex = dxManager_->GetSwapChainManager()->GetCurrentBackBufferIndex();
	//ID3D12Resource* backBuffer = dxManager_->GetSwapChainManager()->GetBackBuffer(backBufferIndex);

	//auto* commandContext = dxManager_->GetCommandContext();

	//// RenderTarget → Present
	//commandContext->TransitionResource(
	//	backBuffer,
	//	D3D12_RESOURCE_STATE_RENDER_TARGET,
	//	D3D12_RESOURCE_STATE_PRESENT
	//);
}

void LightingPath::CreateGBufferSRVs()
{
	auto* srvManager = dxManager_->GetSrvManager();

	// ---- ① 4つ連続の枠を確保（初期化時に1度だけ） ----
	gBufferSrvStartIndex_ = srvManager->Allocate();   // t0
	uint32_t index1 = srvManager->Allocate();         // t1
	uint32_t index2 = srvManager->Allocate();         // t2
	uint32_t index3 = srvManager->Allocate();         // t3

	// ---- ② SRV生成（キャッシュ） ----
	srvManager->CreateSRVforTexture2D(
		gBufferSrvStartIndex_,
		gBufferManager_->GetResource(GBufferManager::GBufferType::Albedo),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		1
	);

	srvManager->CreateSRVforTexture2D(
		index1,
		gBufferManager_->GetResource(GBufferManager::GBufferType::Normal),
		DXGI_FORMAT_R16G16B16A16_FLOAT,
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
		DXGI_FORMAT_R8G8B8A8_UNORM,
		1
	);

	// ---- ③ GPUハンドルをキャッシュ ----
	gBufferSrvTable_ = srvManager->GetGPUDescriptorHandle(gBufferSrvStartIndex_);
}
