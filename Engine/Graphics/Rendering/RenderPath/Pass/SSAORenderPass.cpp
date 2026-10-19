#include "SSAORenderPass.h"
#include "../Impl/GBufferManager.h"
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/Environment/RenderEnvironment.h"
#include "World3D/Camera/CameraManager.h"
#include "World3D/Camera/BaseCamera.h"
#include "Platform/WindowManager.h"
#include "Utility/Logger.h"
#include <cassert>
#include <cstring>

namespace {
constexpr DXGI_FORMAT kAoFormat = DXGI_FORMAT_R8_UNORM;
}

void SSAORenderPass::Initialize(const EngineContext& ctx, GBufferManager* gBufferManager)
{
	ctx_ = ctx;
	gBufferManager_ = gBufferManager;

	viewport_ = { 0.0f, 0.0f, static_cast<float>(WindowManager::kGameWidth), static_cast<float>(WindowManager::kGameHeight), 0.0f, 1.0f };
	scissor_ = { 0, 0, static_cast<LONG>(WindowManager::kGameWidth), static_cast<LONG>(WindowManager::kGameHeight) };

	CreateTargets();
	CreateRootSignature();
	CreatePSOs();

	auto* resourceManager = ctx_.dxManager->GetResourceManager();
	// ResourceManager 経由（Map が返すのは CPU 側の写しで、提出の直前に GPU へ写される）。
	// 直接 Map した upload ヒープへ書くと、CPU と GPU が並行して走るときに GPU が読んでいる最中に書き換えてしまう
	ssaoParamHandle_ = resourceManager->CreateUploadBuffer((sizeof(SSAOParam) + 255) & ~255, L"SSAOParam");
	ssaoParam_ = reinterpret_cast<SSAOParam*>(resourceManager->Map(ssaoParamHandle_));
	blurParamHandle_ = resourceManager->CreateUploadBuffer((sizeof(BlurParam) + 255) & ~255, L"SSAOBlurParam");
	blurParam_ = reinterpret_cast<BlurParam*>(resourceManager->Map(blurParamHandle_));

	// ライティングは毎フレーム t8 を読むので、SSAO を使わないシーンでも有効な SRV を渡しておく
	// （中身は使われない。ライティング側で掛ける強さが 0 になる）
	RenderEnvironment::GetInstance().SetAoSrv(blur_.srv);
}

void SSAORenderPass::CreateTargets()
{
	auto* rtvManager = ctx_.dxManager->GetRtvManager();
	auto* srvManager = ctx_.dxManager->GetSrvManager();

	for (Target* target : { &ao_, &blur_ }) {
		GpuResourceFactory::TextureDesc desc;
		desc.width = WindowManager::kGameWidth;
		desc.height = WindowManager::kGameHeight;
		desc.format = kAoFormat;
		desc.usage = GpuResourceFactory::Usage::RenderTarget;
		desc.clearColor[0] = 1.0f;
		desc.clearColor[1] = 1.0f;
		desc.clearColor[2] = 1.0f;
		desc.clearColor[3] = 1.0f;
		target->resource = ctx_.dxManager->GetResourceFactory()->CreateTexture2D(desc);

		const uint32_t rtvIndex = rtvManager->Allocate();
		rtvManager->CreateRTV(rtvIndex, target->resource.Get());
		target->rtv = rtvManager->GetCPUDescriptorHandle(rtvIndex);

		const uint32_t srvIndex = srvManager->Allocate();
		srvManager->CreateSRVforTexture2D(srvIndex, target->resource.Get(), kAoFormat, 1);
		target->srv = srvManager->GetGPUDescriptorHandle(srvIndex);

		// 生成直後は RENDER_TARGET。白（遮られていない）で埋めてから読み取り状態にしておく
		ctx_.dxManager->GetCommandContext()->ClearRenderTarget(target->rtv, desc.clearColor);
		ctx_.dxManager->GetCommandContext()->TransitionResource(target->resource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}

void SSAORenderPass::CreateRootSignature()
{
	D3D12_DESCRIPTOR_RANGE ranges[2] = {};
	for (int i = 0; i < 2; ++i) {
		ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[i].NumDescriptors = 1;
		ranges[i].BaseShaderRegister = i; // t0, t1
		ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	// 0: b0 定数 / 1: t0 / 2: t1。GBuffer の SRV は並んでいる保証が無いので、1枚ずつテーブルを分ける
	D3D12_ROOT_PARAMETER params[3] = {};
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].Descriptor.ShaderRegister = 0;
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	for (int i = 0; i < 2; ++i) {
		params[1 + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[1 + i].DescriptorTable.NumDescriptorRanges = 1;
		params[1 + i].DescriptorTable.pDescriptorRanges = &ranges[i];
		params[1 + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}

	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.pParameters = params;
	desc.NumParameters = _countof(params);
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; // 頂点バッファを使わない（Fullscreen.VS は頂点番号から組む）

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	hr = ctx_.dxManager->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
	(void)hr;
}

void SSAORenderPass::CreatePSOs()
{
	Microsoft::WRL::ComPtr<IDxcBlob> vs = ctx_.dxManager->CompileShader(L"./resource/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
	assert(vs != nullptr);

	auto create = [&](const wchar_t* psPath, Microsoft::WRL::ComPtr<ID3D12PipelineState>& out) {
		Microsoft::WRL::ComPtr<IDxcBlob> ps = ctx_.dxManager->CompileShader(psPath, L"ps_6_0");
		assert(ps != nullptr);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = rootSignature_.Get();
		psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
		psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = kAoFormat;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		HRESULT hr = ctx_.dxManager->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&out));
		assert(SUCCEEDED(hr));
		(void)hr;
	};
	create(L"./resource/shaders/SSAO.PS.hlsl", ssaoPso_);
	create(L"./resource/shaders/SSAOBlur.PS.hlsl", blurPso_);
}

void SSAORenderPass::DrawFullscreen(ID3D12PipelineState* pso, const Target& output, D3D12_GPU_VIRTUAL_ADDRESS cbv,
	D3D12_GPU_DESCRIPTOR_HANDLE t0, D3D12_GPU_DESCRIPTOR_HANDLE t1)
{
	auto* cmd = ctx_.dxManager->GetCommandList();
	auto* context = ctx_.dxManager->GetCommandContext();

	context->TransitionResource(output.resource.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	context->SetRenderTarget(output.rtv);
	context->SetViewportAndScissor(viewport_, scissor_);

	cmd->SetPipelineState(pso);
	cmd->SetGraphicsRootSignature(rootSignature_.Get());
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->SetGraphicsRootConstantBufferView(0, cbv);
	cmd->SetGraphicsRootDescriptorTable(1, t0);
	cmd->SetGraphicsRootDescriptorTable(2, t1);
	cmd->DrawInstanced(3, 1, 0, 0);

	context->TransitionResource(output.resource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void SSAORenderPass::Execute()
{
	const RenderEnvironment::Params& env = RenderEnvironment::GetInstance().GetParams();
	if (!env.ssaoEnabled || !ssaoParam_ || !blurParam_) {
		return;
	}
	BaseCamera* camera = CameraManager::GetInstance().GetActiveCamera();
	if (!camera) {
		return;
	}

	SSAOParam param{};
	std::memcpy(param.viewProj, &camera->GetViewProjectionMatrix(), sizeof(param.viewProj));
	// 行ベクトルの規約なので平行移動は4行目
	const Matrix4x4& camWorld = camera->GetWorldMatrix();
	param.cameraPos[0] = camWorld.m[3][0];
	param.cameraPos[1] = camWorld.m[3][1];
	param.cameraPos[2] = camWorld.m[3][2];
	param.radius = env.ssaoRadius;
	param.bias = env.ssaoBias;
	param.intensity = env.ssaoIntensity;
	param.power = env.ssaoPower;
	*ssaoParam_ = param;
	blurParam_->depthSharpness = 4.0f;

	ID3D12DescriptorHeap* heaps[] = { ctx_.dxManager->GetSrvManager()->GetHeap() };
	ctx_.dxManager->GetCommandList()->SetDescriptorHeaps(1, heaps);

	const D3D12_GPU_DESCRIPTOR_HANDLE normalSrv = gBufferManager_->GetSRVHandle(GBufferManager::GBufferType::Normal);
	const D3D12_GPU_DESCRIPTOR_HANDLE worldPosSrv = gBufferManager_->GetSRVHandle(GBufferManager::GBufferType::WorldPos);

	DrawFullscreen(ssaoPso_.Get(), ao_, ctx_.dxManager->GetResourceManager()->GetGPUVirtualAddress(ssaoParamHandle_), normalSrv, worldPosSrv);
	DrawFullscreen(blurPso_.Get(), blur_, ctx_.dxManager->GetResourceManager()->GetGPUVirtualAddress(blurParamHandle_), ao_.srv, worldPosSrv);
}
