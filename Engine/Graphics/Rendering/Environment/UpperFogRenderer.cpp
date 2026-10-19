#include "UpperFogRenderer.h"
#include "RenderEnvironment.h"
#include "Graphics/Device/DirectXManager.h"
#include "World3D/Camera/CameraManager.h"
#include "World3D/Camera/BaseCamera.h"
#include "Utility/TimeManager.h"
#include "Utility/Logger.h"
#include <cassert>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <iterator>

UpperFogRenderer& UpperFogRenderer::GetInstance()
{
	static UpperFogRenderer instance;
	return instance;
}

void UpperFogRenderer::Initialize(DirectXManager* dxManager)
{
	dxManager_ = dxManager;
	CreateRootSignature();
	CreatePSO();

	// ResourceManager 経由（Map が返すのは CPU 側の写しで、提出の直前に GPU へ写される）。
	// 直接 Map した upload ヒープへ書くと、CPU と GPU が並行して走るときに GPU が読んでいる最中に書き換えてしまう
	auto* resourceManager = dxManager_->GetResourceManager();
	bufferHandle_ = resourceManager->CreateUploadBuffer(kSlotSize * kMaxLayers, L"UpperFog");
	mapped_ = reinterpret_cast<unsigned char*>(resourceManager->Map(bufferHandle_));
	std::memset(mapped_, 0, kSlotSize * kMaxLayers);
}

void UpperFogRenderer::Finalize()
{
	if (dxManager_ && bufferHandle_ != kInvalidBufferHandle) {
		dxManager_->GetResourceManager()->ReleaseBuffer(bufferHandle_);
	}
	bufferHandle_ = kInvalidBufferHandle;
	mapped_ = nullptr;
	pso_.Reset();
	rootSignature_.Reset();
}

void UpperFogRenderer::CreateRootSignature()
{
	// t0: GBuffer のワールド座標（壁や岩と交わる所を薄くするのに使う）
	D3D12_DESCRIPTOR_RANGE range{};
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.NumDescriptors = 1;
	range.BaseShaderRegister = 0;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER params[2] = {};
	// 0: b0 層ごとの設定（VS / PS）
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].Descriptor.ShaderRegister = 0;
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	// 1: t0
	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[1].DescriptorTable.NumDescriptorRanges = 1;
	params[1].DescriptorTable.pDescriptorRanges = &range;
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.pParameters = params;
	desc.NumParameters = _countof(params);
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; // 頂点バッファを使わない

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	hr = dxManager_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
	(void)hr;
}

void UpperFogRenderer::CreatePSO()
{
	// 乗算済みアルファ。アルファは書き換えない（フォワードのRTのアルファを他のパスが見ているため）
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;

	// 下から見上げることが多いので両面描く
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.DepthClipEnable = TRUE;

	// 深度は見るが書かない
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	Microsoft::WRL::ComPtr<IDxcBlob> vs = dxManager_->CompileShader(L"./resource/shaders/FogLayer.VS.hlsl", L"vs_6_0");
	assert(vs != nullptr);
	Microsoft::WRL::ComPtr<IDxcBlob> ps = dxManager_->CompileShader(L"./resource/shaders/FogLayer.PS.hlsl", L"ps_6_0");
	assert(ps != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
	psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
	psoDesc.BlendState = blendDesc;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.DepthStencilState = depthStencilDesc;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	HRESULT hr = dxManager_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
	assert(SUCCEEDED(hr));
	(void)hr;
}

void UpperFogRenderer::Draw()
{
	if (!mapped_ || !pso_) return;

	time_ += TimeManager::GetVFXDelta();

	// デバッグ表示（光の筋だけ・埃だけ）のときは霧の層を描かない（上に重なって見づらいので）
	if (RenderEnvironment::GetInstance().GetDebugView() != RenderEnvironment::DebugView::None) return;

	const RenderEnvironment::Params& env = RenderEnvironment::GetInstance().GetParams();
	bool any = false;
	for (const auto& layer : env.fogLayers) {
		any = any || (layer.enabled && layer.opacity > 0.0f);
	}
	if (!any) return;

	BaseCamera* camera = CameraManager::GetInstance().GetActiveCamera();
	if (!camera) return;

	auto* cmd = dxManager_->GetCommandList();
	cmd->SetPipelineState(pso_.Get());
	cmd->SetGraphicsRootSignature(rootSignature_.Get());
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->SetGraphicsRootDescriptorTable(1, sceneWorldPosSrv_);

	const Matrix4x4& viewProj = camera->GetViewProjectionMatrix();
	const Matrix4x4& camWorld = camera->GetWorldMatrix();

	// カメラから高さが離れている層から描く（見上げる上部の霧も、見下ろす地面の霧も、遠い順に重なる）
	const float cameraY = camWorld.m[3][1];
	int order[RenderEnvironment::Params::kFogLayerCount];
	for (int i = 0; i < RenderEnvironment::Params::kFogLayerCount; ++i) order[i] = i;
	std::sort(std::begin(order), std::end(order), [&](int a, int b) {
		return std::abs(env.fogLayers[a].height - cameraY) > std::abs(env.fogLayers[b].height - cameraY);
	});

	for (int i : order) {
		const auto& layer = env.fogLayers[i];
		if (!layer.enabled || layer.opacity <= 0.0f || i >= kMaxLayers) continue;

		FogLayerCB cb{};
		std::memcpy(cb.viewProj, &viewProj, sizeof(cb.viewProj));
		cb.center[0] = env.fogLayerCenter.x;
		cb.center[1] = layer.height;
		cb.center[2] = env.fogLayerCenter.z;
		cb.halfSize = env.fogLayerHalfSize;
		const Vector3 color = RenderEnvironment::ToLinear(layer.color);
		cb.color[0] = color.x;
		cb.color[1] = color.y;
		cb.color[2] = color.z;
		cb.opacity = layer.opacity;
		// 行ベクトルの規約なので平行移動は4行目
		cb.cameraPos[0] = camWorld.m[3][0];
		cb.cameraPos[1] = camWorld.m[3][1];
		cb.cameraPos[2] = camWorld.m[3][2];
		cb.time = time_;
		cb.noiseOffset[0] = layer.noiseSpeed.x * time_;
		cb.noiseOffset[1] = layer.noiseSpeed.z * time_;
		cb.noiseScale = layer.noiseScale;
		cb.softDistance = layer.softDistance >= 0.0f ? layer.softDistance : env.fogLayerSoftDistance;
		cb.grazingFade = layer.grazingFade ? 1.0f : 0.0f;
		cb.edgeFade = env.fogLayerEdgeFade;
		cb.layerSeed = static_cast<float>(i) * 17.31f;

		std::memcpy(mapped_ + kSlotSize * i, &cb, sizeof(cb));
		cmd->SetGraphicsRootConstantBufferView(0, dxManager_->GetResourceManager()->GetGPUVirtualAddress(bufferHandle_) + kSlotSize * i);
		cmd->DrawInstanced(6, 1, 0, 0);
	}
}
