#include "CascadedShadowMap.h"
#include "Graphics/Device/DirectXManager.h"
#include "3d/Camera/BaseCamera.h"
#include "3d/Light/LightManager.h"
#include "3d/Camera/CameraManager.h"
#include <math/Vector4.h>
#include <math/function.h>
#include <algorithm>
#include <cfloat>
#ifdef _DEBUG
#include <imgui.h>
#endif

void CascadedShadowMap::Initialize(DirectXManager* dxManager, uint32_t shadowMapSize) {
	dxManager_ = dxManager;
	shadowMapSize_ = shadowMapSize;

	CreateDSV();

	auto* resourceManager = dxManager_->GetResourceManager();
	for (uint32_t i = 0; i < kCascadeCount; ++i) {
		lightVPIndex_[i] = resourceManager->CreateUploadBuffer(sizeof(LightData) * 128);
		mappedLightVP_[i] = reinterpret_cast<LightVPConstants*>(resourceManager->Map(lightVPIndex_[i]));
	}

	// ライティングパス用: 全カスケードデータをまとめた CB
	lightingCBIndex_  = resourceManager->CreateUploadBuffer(512); // 272B data, 256B-aligned allocation
	mappedLightingCB_ = reinterpret_cast<CascadeLightingCB*>(resourceManager->Map(lightingCBIndex_));
}

void CascadedShadowMap::Update() {
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

	// シャドウパス用（各カスケードの VP 行列を個別バッファに書き込み）
	for (uint32_t i = 0; i < kCascadeCount; ++i) {
		mappedLightVP_[i]->lightViewProj = cascades_[i].lightViewProj;
	}

	// ライティングパス用（全カスケードデータをまとめて書き込み）
	mappedLightingCB_->lightViewProj0   = cascades_[0].lightViewProj;
	mappedLightingCB_->lightViewProj1   = cascades_[1].lightViewProj;
	mappedLightingCB_->lightViewProj2   = cascades_[2].lightViewProj;
	mappedLightingCB_->cascadeFar[0]    = cascades_[0].splitDepth;
	mappedLightingCB_->cascadeFar[1]    = cascades_[1].splitDepth;
	mappedLightingCB_->cascadeFar[2]    = cascades_[2].splitDepth;
	mappedLightingCB_->cascadeFar[3]    = 0.0f;
	mappedLightingCB_->cameraView       = camera_->GetViewMatrix();
}

void CascadedShadowMap::Bind(uint32_t rootIndex, uint32_t cascadeIndex) {
	auto* cmd = dxManager_->GetCommandContext()->GetCommandList();
	auto* resourceManager = dxManager_->GetResourceManager();

	// b1 : Light View Projection（この Cascade 用）
	cmd->SetGraphicsRootConstantBufferView(rootIndex, resourceManager->GetGPUVirtualAddress(lightVPIndex_[cascadeIndex]));
}

void CascadedShadowMap::BeginCascade(uint32_t index) {
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

void CascadedShadowMap::EndCascade(uint32_t index) {
	dxManager_->GetCommandContext()->TransitionResource(
		shadowMaps_[index].Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
}

void CascadedShadowMap::BindSrv() {
	auto* cmd = dxManager_->GetCommandContext()->GetCommandList();
	auto* srvManager = dxManager_->GetSrvManager();

	// srvIndices_[0~2] は連続確保済みのためテーブル先頭だけ指定すれば t5,t6,t7 が全て対応する
	cmd->SetGraphicsRootDescriptorTable(4, srvManager->GetGPUDescriptorHandle(srvIndices_[0]));
}

void CascadedShadowMap::BindCascadeCB(uint32_t rootIndex) {
	auto* cmd             = dxManager_->GetCommandContext()->GetCommandList();
	auto* resourceManager = dxManager_->GetResourceManager();

	cmd->SetGraphicsRootConstantBufferView(rootIndex, resourceManager->GetGPUVirtualAddress(lightingCBIndex_));
}

void CascadedShadowMap::CalculateCascadeSplits() {
	if (!camera_) return;

	float nearVal = camera_->GetNearClip();
	float farVal  = shadowFar_;

	// Practical Split Scheme: 対数分割と均等分割を splitLambda_ でブレンド
	for (uint32_t i = 1; i <= kCascadeCount; ++i) {
		float t         = float(i) / float(kCascadeCount);
		float logSplit  = nearVal * powf(farVal / nearVal, t);
		float uniSplit  = nearVal + (farVal - nearVal) * t;
		cascades_[i - 1].splitDepth = splitLambda_ * logSplit + (1.0f - splitLambda_) * uniSplit;
	}
	// 最終カスケードは必ず shadowFar_ まで
	cascades_[kCascadeCount - 1].splitDepth = farVal;
}

void CascadedShadowMap::CalculateLightMatrices() {
	if (!camera_) return;
	if (lights_.empty()) return;

	Vector3 lightDir = Normalize(lights_[0].direction);

	Vector3 up = { 0.0f, 1.0f, 0.0f };
	if (fabsf(Dot(lightDir, up)) > 0.999f) {
		up = { 0.0f, 0.0f, 1.0f };
	}

	float fovY   = camera_->GetFovY();
	float aspect = camera_->GetAspectRate();
	const Matrix4x4& viewMatrix = camera_->GetViewMatrix();

	float cascadeNear = camera_->GetNearClip();

	for (uint32_t cascIdx = 0; cascIdx < kCascadeCount; ++cascIdx) {
		float nearZ = cascadeNear;
		float farZ  = cascades_[cascIdx].splitDepth;
		cascadeNear = farZ;  // 次のカスケードの near

		// このカスケード用の VP 逆行列でワールド空間フラスタムコーナーを取得
		Matrix4x4 cascadeProj = MakePerspectiveFovMatrix(fovY, aspect, nearZ, farZ);
		Matrix4x4 invVP       = Inverse(viewMatrix * cascadeProj);

		// NDC 8頂点 (DX12: Z=[0,1]) → ワールド空間
		Vector3 corners[8];
		int idx = 0;
		for (int zi = 0; zi < 2; ++zi) {
			float nz = (zi == 0) ? 0.0f : 1.0f;
			for (int yi = 0; yi < 2; ++yi) {
				float ny = (yi == 0) ? -1.0f : 1.0f;
				for (int xi = 0; xi < 2; ++xi) {
					float nx = (xi == 0) ? -1.0f : 1.0f;
					Vector4 ndc   = { nx, ny, nz, 1.0f };
					Vector4 world = ndc * invVP;
					corners[idx++] = { world.x / world.w, world.y / world.w, world.z / world.w };
				}
			}
		}

		// フラスタムコーナーの重心を注視点とするライトビュー行列を生成
		Vector3 center = {};
		for (int i = 0; i < 8; ++i) {
			center.x += corners[i].x;
			center.y += corners[i].y;
			center.z += corners[i].z;
		}
		center.x /= 8.0f;
		center.y /= 8.0f;
		center.z /= 8.0f;

		Matrix4x4 lightView = CreateLookAtMatrix(center - lightDir * shadowDistance_, center, up);

		// ライトビュー空間でのAABBを計算してタイトなオルソ投影を生成
		float minX = FLT_MAX, maxX = -FLT_MAX;
		float minY = FLT_MAX, maxY = -FLT_MAX;
		float minZ = FLT_MAX, maxZ = -FLT_MAX;
		for (int i = 0; i < 8; ++i) {
			Vector3 lv = Transform(corners[i], lightView);
			minX = (std::min)(minX, lv.x); maxX = (std::max)(maxX, lv.x);
			minY = (std::min)(minY, lv.y); maxY = (std::max)(maxY, lv.y);
			minZ = (std::min)(minZ, lv.z); maxZ = (std::max)(maxZ, lv.z);
		}

		// 影の送り主（カメラ後方のオブジェクト）を含めるため near を後退させる
		minZ -= shadowDistance_;

		Matrix4x4 lightProj = MakeOrthographicMatrix(minX, maxY, maxX, minY, minZ, maxZ);
		cascades_[cascIdx].lightViewProj = lightView * lightProj;
	}
}

void CascadedShadowMap::GetFrustumCornersViewSpace(float fovY, float aspect, float nearZ, float farZ, Vector3 outCorners[8]) {
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

void CascadedShadowMap::CreateDSV() {
	for (uint32_t i = 0; i < kCascadeCount; ++i) {
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

#ifdef _DEBUG
void CascadedShadowMap::DrawDebugUI() {
	ImGui::Begin("Shadow Map (CSM)");

	ImGui::Text("Parameters");
	ImGui::Separator();
	ImGui::DragFloat("Light Distance", &shadowDistance_, 1.0f, 10.0f, 500.0f, "%.1f");
	ImGui::DragFloat("Shadow Far",     &shadowFar_,      1.0f, 1.0f,  2000.0f, "%.1f");
	ImGui::SliderFloat("Split Lambda", &splitLambda_,    0.0f, 1.0f,  "%.2f");

	ImGui::Spacing();
	ImGui::Text("Cascade Split Depths (view-space Z)");
	ImGui::Separator();
	for (uint32_t i = 0; i < kCascadeCount; ++i) {
		ImGui::Text("  Cascade %u far: %.1f", i, cascades_[i].splitDepth);
	}

	if (!lights_.empty()) {
		ImGui::Spacing();
		ImGui::Text("Light Direction");
		ImGui::Separator();
		ImGui::Text("  (%.2f, %.2f, %.2f)", lights_[0].direction.x, lights_[0].direction.y, lights_[0].direction.z);
	}

	if (camera_) {
		ImGui::Spacing();
		Vector3 p = camera_->GetTranslate();
		ImGui::Text("Camera Pos");
		ImGui::Separator();
		ImGui::Text("  (%.1f, %.1f, %.1f)", p.x, p.y, p.z);
	}

	ImGui::End();
}
#endif
