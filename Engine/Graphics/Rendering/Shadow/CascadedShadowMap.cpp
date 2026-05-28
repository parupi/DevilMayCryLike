#include "CascadedShadowMap.h"
#include "Graphics/Device/DirectXManager.h"
#include "3d/Camera/BaseCamera.h"
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
}

void CascadedShadowMap::Update()
{
	// カメラの情報を取得
	camera_ = CameraManager::GetInstance()->GetActiveCamera();
	if (!camera_) return;

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

	for (uint32_t i = 0; i < kCascadeCount; ++i)
	{
		mappedLightVP_[i]->lightViewProj = cascades_[i].lightViewProj;
	}
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

	ctx->GetCommandList()->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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

void CascadedShadowMap::BindSrv()
{
	auto* cmd = dxManager_->GetCommandContext()->GetCommandList();
	auto* srvManager = dxManager_->GetSrvManager();

	cmd->SetGraphicsRootDescriptorTable(4, srvManager->GetGPUDescriptorHandle(srvIndices_[0]));
}

void CascadedShadowMap::CalculateCascadeSplits()
{
	// 固定オルソ投影を使うため分割計算は不要
}

void CascadedShadowMap::CalculateLightMatrices()
{
	if (!camera_) return;
	if (lights_.empty()) return;

	Vector3 lightDir = Normalize(lights_[0].direction);

	// lightDir が真上/真下の場合は up を前方向にする
	Vector3 up = { 0.0f, 1.0f, 0.0f };
	if (fabsf(Dot(lightDir, up)) > 0.999f) {
		up = { 0.0f, 0.0f, 1.0f };
	}

	// カメラ位置を中心にシャドウマップを配置
	Vector3 center = camera_->GetTranslate();
	const float shadowDistance = 150.0f;
	const float orthoSize      = 100.0f;  // 中心から50ユニット四方をカバー
	const float shadowNear     = 0.1f;
	const float shadowFar      = shadowDistance * 2.0f;

	Matrix4x4 lightView = CreateLookAtMatrix(
		center - lightDir * shadowDistance,
		center,
		up
	);

	Matrix4x4 lightProj = CreateOrthographic(orthoSize, orthoSize, shadowNear, shadowFar);

	Matrix4x4 lightVP = lightView * lightProj;

	for (uint32_t i = 0; i < kCascadeCount; ++i) {
		cascades_[i].lightViewProj = lightVP;
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
		desc.width = shadowMapSize_;
		desc.height = shadowMapSize_;
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
