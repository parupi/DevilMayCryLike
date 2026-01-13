#include "CascadedShadowMap.h"
#include "Graphics/Device/DirectXManager.h"
#include "3d/Camera/Camera.h"
#include "3d/Light/LightManager.h"
#include "3d/Camera/CameraManager.h"

void CascadedShadowMap::Initialize(DirectXManager* dxManager, uint32_t shadowMapSize)
{
	dxManager_ = dxManager;
	shadowMapSize_ = shadowMapSize;

	CreateDSV();

	auto* resourceManager = dxManager_->GetResourceManager();
	for (uint32_t i = 0; i < kCascadeCount; ++i)
	{
		lightVPIndex_[i] = resourceManager->CreateUploadBuffer(sizeof(LightData) * 128);

		mappedLightVP_[i] = reinterpret_cast<LightVPConstants*>(resourceManager->Map(lightVPIndex_[i]));
	}

	shadowMapSrvIndex_ = srvIndices_[0];
	cascadeCBHandle_ = 
	lightVPCBHandle_ = lightVPIndex_[0];
}

void CascadedShadowMap::Update()
{
	// カメラの情報を取得
	camera_ = CameraManager::GetInstance()->GetActiveCamera();
	// ライトの情報を取得
	lights_ = LightManager::GetInstance()->GetAllLightData();
	// DirectionalLight以外を削除
	lights_.erase(
		std::remove_if(
			lights_.begin(),
			lights_.end(),
			[](const LightData& light) {
				return light.type != 0; // 消したい条件
			}
		),
		lights_.end()
	);

	CalculateCascadeSplits();
	CalculateLightMatrices();
}

void CascadedShadowMap::Bind(uint32_t rootIndex, uint32_t cascadeIndex)
{
	auto* cmd = dxManager_->GetCommandContext()->GetCommandList();
	auto* resourceManager = dxManager_->GetResourceManager();

	// b1 : Light View Projection（この Cascade 用）
	cmd->SetGraphicsRootConstantBufferView(rootIndex, resourceManager->GetGPUVirtualAddress(lightVPIndex_[cascadeIndex]));
}

void CascadedShadowMap::BeginCascade(uint32_t index)
{
	auto* ctx = dxManager_->GetCommandContext();

	ctx->TransitionResource(
		shadowMaps_[index].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);

	D3D12_VIEWPORT vp{};
	vp.Width = (float)shadowMapSize_;
	vp.Height = (float)shadowMapSize_;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	D3D12_RECT scissor{ 0,0,(LONG)shadowMapSize_,(LONG)shadowMapSize_ };
	ctx->SetViewportAndScissor(vp, scissor);

	auto dsv = dxManager_->GetDsvManager()->GetDsvHandle(dsvIndices_[index]);

	ctx->SetRenderTargets(nullptr, 0, &dsv);
}

void CascadedShadowMap::EndCascade(uint32_t index)
{
	dxManager_->GetCommandContext()->TransitionResource(
		shadowMaps_[index].Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
}

void CascadedShadowMap::CalculateCascadeSplits()
{
	float nearZ = camera_->GetNearClip();
	float farZ = camera_->GetFarClip();

	cascades_[0].splitDepth = nearZ + 10.0f;
	cascades_[1].splitDepth = nearZ + 30.0f;
	cascades_[2].splitDepth = farZ;
}

void CascadedShadowMap::CalculateLightMatrices()
{
	float prevSplit = camera_->GetNearClip();

	for (uint32_t i = 0; i < kCascadeCount; ++i)
	{
		float split = cascades_[i].splitDepth;

		// Frustum8点を取得
		Vector3 frustumView[8];
		GetFrustumCornersViewSpace(camera_->GetFovY(), camera_->GetAspect(), prevSplit, split, frustumView);

		// ViewからWorldに変換
		Matrix4x4 invView = Inverse(camera_->GetViewMatrix());
		Vector3 frustumWorld[8]{};

		for (int j = 0; j < 8; ++j) {
			frustumWorld[j] = Transform(frustumView[j], invView);
		}

		Vector3 center{ 0.0f, 0.0f, 0.0f };
		for (auto& v : frustumWorld) {
			center += v;
		}
		center /= 8.0f;

		// World → LightView
		Vector3 lightDir = Normalize(lights_[0].direction);

		Matrix4x4 lightView = CreateLookAtMatrix(center - lightDir * 100.0f, center, { 0.0f, 1.0f, 0.0f });

		Vector3 frustumLight[8]{};
		for (int j = 0; j < 8; ++j) {
			frustumLight[j] = Transform(frustumWorld[j], lightView);
		}

		Vector3 min = frustumLight[0];
		Vector3 max = frustumLight[0];
		for (int j = 1; j < 8; ++j) {
			min = Min(min, frustumLight[j]);
			max = Max(max, frustumLight[j]);
		}

		Matrix4x4 lightProj = CreateOrthographic(max.x - min.x, max.y - min.y, min.z, max.z);

		cascades_[i].lightViewProj = lightView * lightProj;

		prevSplit = split;
	}
}

void CascadedShadowMap::GetFrustumCornersViewSpace(float fovY, float aspect, float nearZ, float farZ, Vector3 outCorners[8])
{
	float tanFov = tanf(fovY * 0.5f);

	float nearH = nearZ * tanFov;
	float nearW = nearH * aspect;
	float farH = farZ * tanFov;
	float farW = farH * aspect;

	// Near
	outCorners[0] = { -nearW,  nearH, nearZ };
	outCorners[1] = { nearW,  nearH, nearZ };
	outCorners[2] = { nearW, -nearH, nearZ };
	outCorners[3] = { -nearW, -nearH, nearZ };

	// Far
	outCorners[4] = { -farW,  farH, farZ };
	outCorners[5] = { farW,  farH, farZ };
	outCorners[6] = { farW, -farH, farZ };
	outCorners[7] = { -farW, -farH, farZ };
}

void CascadedShadowMap::CreateDSV()
{
	for (uint32_t i = 0; i < kCascadeCount; ++i)
	{
		GpuResourceFactory::TextureDesc desc{};
		desc.format = DXGI_FORMAT_R24G8_TYPELESS;
		desc.usage = GpuResourceFactory::Usage::DepthStencil;

		shadowMaps_[i] = dxManager_->GetResourceFactory()->CreateTexture2D(desc);

		auto* dsvMgr = dxManager_->GetDsvManager();
		dsvIndices_[i] = dsvMgr->Allocate();
		dsvMgr->CreateDsv(dsvIndices_[i], shadowMaps_[i].Get(), DXGI_FORMAT_D24_UNORM_S8_UINT);

		auto* srvMgr = dxManager_->GetSrvManager();
		srvIndices_[i] = srvMgr->Allocate();
		srvMgr->CreateSRVforTexture2D(srvIndices_[i], shadowMaps_[i].Get(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS, 1);

		dxManager_->GetCommandContext()->TransitionResource(
			shadowMaps_[i].Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
	}
}
