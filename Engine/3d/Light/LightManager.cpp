#include "LightManager.h"
#include <3d/Object/Object3dManager.h>

LightManager* LightManager::instance = nullptr;
std::once_flag LightManager::initInstanceFlag;

LightManager* LightManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new LightManager();
		});
	return instance;
}

void LightManager::Initialize(DirectXManager* dxManager)
{
	dxManager_ = dxManager;

	CreateLightBuffer();
}

void LightManager::Finalize()
{
	gpuLightCache_.clear();
	lights_.clear();

	if (lightCountBuffer_) lightCountBuffer_.Reset();
	if (lightBuffer_) lightBuffer_.Reset();

	dxManager_ = nullptr;

	if (instance) {
		delete instance;
		instance = nullptr;
	}
}

void LightManager::Update()
{
	gpuLightCache_.clear();
	gpuLightCache_.reserve(lights_.size());

	// GPUに送る構造体を取得
	for (auto& light : lights_)
	{
		light->Update();
		LightData data = light->GetLightData();
		gpuLightCache_.push_back(data);
	}

	UploadToGPU();
}

void LightManager::AddLight(std::unique_ptr<BaseLight> light)
{
	lights_.push_back(std::move(light));
}

void LightManager::DeleteAllLight()
{
	lights_.clear();
}

void LightManager::UploadToGPU()
{
	// LightBuffer にコピー
	void* mapped = nullptr;
	lightBuffer_->Map(0, nullptr, &mapped);
	assert(gpuLightCache_.size() <= MaxLights);
	memcpy(mapped, gpuLightCache_.data(), sizeof(LightData) * gpuLightCache_.size());
	lightBuffer_->Unmap(0, nullptr);

	// LightCount ConstantBuffer も更新
	UINT* countMapped = nullptr;
	lightCountBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&countMapped));
	*countMapped = static_cast<UINT>(gpuLightCache_.size());
	lightCountBuffer_->Unmap(0, nullptr);
}

void LightManager::CreateLightBuffer()
{
	// Light buffer (StructuredBuffer)
	dxManager_->CreateBufferResource(sizeof(LightData) * MaxLights, lightBuffer_);

	// LightCount (CBV)
	UINT cbSize = (sizeof(UINT) + 255) & ~255;
	dxManager_->CreateBufferResource(cbSize, lightCountBuffer_);

	// SRV 作成
	auto* srvManager = dxManager_->GetSrvManager();
	srvIndex_ = srvManager->Allocate();
	srvManager->CreateSRVforStructuredBuffer(srvIndex_, lightBuffer_.Get(), MaxLights, sizeof(LightData));
}

void LightManager::BindLightsToShader()
{
	auto commandList = dxManager_->GetCommandList();
	auto* srvManager = dxManager_->GetSrvManager();

	// LightCount
	commandList->SetGraphicsRootConstantBufferView(2, lightCountBuffer_->GetGPUVirtualAddress());

	// Lights SRV table
	commandList->SetGraphicsRootDescriptorTable(3, srvManager->GetGPUDescriptorHandle(srvIndex_));
}
