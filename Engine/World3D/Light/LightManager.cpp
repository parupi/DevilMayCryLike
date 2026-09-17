#include "LightManager.h"
#include <cassert>
#include <mutex>
#include <Math/MathUtils.h>
#include <World3D/Primitive/PrimitiveLineDrawer.h>
#ifdef _DEBUG
// デバッグ描画のON/OFFゲートだけ。ライトのUIは Engine/Editor/Windows/LightWindow.cpp にある
#include "Editor/Core/EditorDebugDraw.h"
#endif

LightManager& LightManager::GetInstance() {
	static LightManager instance;
	return instance;
}

void LightManager::Initialize(DirectXManager* dxManager) {
	dxManager_ = dxManager;
	CreateLightBuffers();

	csm = std::make_unique<CascadedShadowMap>();
	csm->Initialize(dxManager, 1280);
}

void LightManager::Finalize() {
	lights_.clear();
	gpuLightCache_.clear();

	csm.reset();

	if (dxManager_) {
		auto* rm = dxManager_->GetResourceManager();
		if (lightBufferHandle_ != kInvalidBufferHandle) {
			rm->ReleaseBuffer(lightBufferHandle_);
			lightBufferHandle_ = kInvalidBufferHandle;
		}
		if (lightCountHandle_ != kInvalidBufferHandle) {
			rm->ReleaseBuffer(lightCountHandle_);
			lightCountHandle_ = kInvalidBufferHandle;
		}
	}

	mappedLightPtr_ = nullptr;
	mappedCountPtr_ = nullptr;

	dxManager_ = nullptr;
}

void LightManager::Update() {
	gpuLightCache_.clear();
	gpuLightCache_.reserve(lights_.size());

	for (auto& light : lights_) {
		light->Update();
		gpuLightCache_.push_back(light->GetLightData());
	}

	// --- GPU バッファ更新（永続 Map なので memcpy だけ） ---
	assert(gpuLightCache_.size() <= MaxLights);

	memcpy(mappedLightPtr_, gpuLightCache_.data(), sizeof(LightData) * gpuLightCache_.size());

	*mappedCountPtr_ = static_cast<UINT>(gpuLightCache_.size());

	csm->Update();
	// ライトとCSMのUIは Engine/Editor/Windows/ 側（LightWindow / RenderWindow）が描く
}

BaseLight* LightManager::AddLight(std::unique_ptr<BaseLight> light) {
	lights_.push_back(std::move(light));
	return lights_.back().get();
}

void LightManager::RemoveLight(BaseLight* light) {
	if (!light) return;
	std::erase_if(lights_, [light](const std::unique_ptr<BaseLight>& l) { return l.get() == light; });
}

void LightManager::DeleteAllLight() {
	lights_.clear();
}

void LightManager::BindLightsToShader() {
	auto* cmd = dxManager_->GetCommandList();
	auto* srv = dxManager_->GetSrvManager();
	auto* rm = dxManager_->GetResourceManager();

	// Count CBV
	cmd->SetGraphicsRootConstantBufferView(2, rm->GetGPUVirtualAddress(lightCountHandle_));

	// SRV (StructuredBuffer)
	cmd->SetGraphicsRootDescriptorTable(3, srv->GetGPUDescriptorHandle(srvIndex_));
}

#ifdef _DEBUG
void LightManager::DrawDebug() {
	// 表示のON/OFFはエディタのDebug Drawメニューに集約している
	if (!EditorDebugDraw::IsEnabled(EditorDebugDraw::Flag::LightGizmo)) {
		return;
	}
	for (auto& light : lights_) {
		if (!light) continue;
		light->DrawDebug(&PrimitiveLineDrawer::GetInstance());
	}
}
#endif // DEBUG

void LightManager::CreateLightBuffers() {
	auto* rm = dxManager_->GetResourceManager();

	// --- 1. Light StructuredBuffer (UPLOAD) ---
	lightBufferHandle_ = rm->CreateUploadBuffer(sizeof(LightData) * MaxLights, L"LightData");

	// 永続 Map
	mappedLightPtr_ = reinterpret_cast<LightData*>(rm->Map(lightBufferHandle_));

	// --- 2. LightCount ConstantBuffer (UPLOAD) ---
	UINT cbSize = (sizeof(UINT) + 255) & ~255;

	lightCountHandle_ = rm->CreateUploadBuffer(cbSize, L"LightCount");

	mappedCountPtr_ = reinterpret_cast<UINT*>(rm->Map(lightCountHandle_));

	// --- 3. SRV 作成 ---
	auto* srv = dxManager_->GetSrvManager();
	srvIndex_ = srv->Allocate();

	srv->CreateSRVforStructuredBuffer(srvIndex_, rm->GetResource(lightBufferHandle_), MaxLights, sizeof(LightData));
}

