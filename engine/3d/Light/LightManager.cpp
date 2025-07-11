#include "LightManager.h"
#include <3d/Object/Object3dManager.h>
#include <numbers>

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

	CreateLightResource();
}

void LightManager::Finalize()
{
	dirLights_.clear();
	spotLights_.clear();
	pointLights_.clear();

	if (dirLightResource_) {
		dirLightResource_.Reset();
	}
	if (spotLightResource_) {
		spotLightResource_.Reset();
	}
	if (pointLightResource_) {
		pointLightResource_.Reset();
	}

	dxManager_ = nullptr;

	delete instance;
	instance = nullptr;
}

void LightManager::BindLightsToShader()
{
	auto commandList = dxManager_->GetCommandList();

	// ディレクションライトのリソースをシェーダーにバインド
	commandList->SetGraphicsRootConstantBufferView(4, dirLightResource_->GetGPUVirtualAddress());
	// ポイントライトのリソースをシェーダーにバインド
	commandList->SetGraphicsRootConstantBufferView(5, pointLightResource_->GetGPUVirtualAddress());
	// スポットライトのリソースをシェーダーにバインド
	commandList->SetGraphicsRootConstantBufferView(6, spotLightResource_->GetGPUVirtualAddress());
}

void LightManager::UpdateAllLight()
{
	CreateLightResource();

	std::vector<DirectionalLightData> dirData;
	for (auto& light : dirLights_) dirData.push_back(light->GetLightData());
	UpdateBuffer(dirLightResource_.Get(), dirData.data(), sizeof(DirectionalLightData) * dirData.size());
	// ログ出力
	Logger::LogBufferCreation("Light:Directional", dirLightResource_.Get(), sizeof(DirectionalLightData));

	std::vector<PointLightData> pointData;
	for (auto& light : pointLights_) pointData.push_back(light->GetLightData());
	UpdateBuffer(pointLightResource_.Get(), pointData.data(), sizeof(PointLightData) * pointData.size());
	// ログ出力
	Logger::LogBufferCreation("Light:Point", pointLightResource_.Get(), sizeof(PointLightData));

	std::vector<SpotLightData> spotData;
	for (auto& light : spotLights_) spotData.push_back(light->GetLightData());
	UpdateBuffer(spotLightResource_.Get(), spotData.data(), sizeof(SpotLightData) * spotData.size());
	// ログ出力
	Logger::LogBufferCreation("Light:Spot", spotLightResource_.Get(), sizeof(SpotLightData));
}

void LightManager::AddDirectionalLight(std::unique_ptr<DirectionalLight> light)
{
	dirLights_.push_back(std::move(light));
}

void LightManager::AddPointLight(std::unique_ptr<PointLight> light)
{
	pointLights_.push_back(std::move(light));
}

void LightManager::AddSpotLight(std::unique_ptr<SpotLight> light)
{
	spotLights_.push_back(std::move(light));
}

DirectionalLight* LightManager::GetDirectionalLight(const std::string& name_)
{
	for (auto& light : dirLights_) {
		if (light->name_ == name_) {
			return light.get();
		}
	}
	return nullptr;
}

PointLight* LightManager::GetPointLight(const std::string& name_)
{
	for (auto& light : pointLights_) {
		if (light->name_ == name_) {
			return light.get();
		}
	}
	return nullptr;
}

SpotLight* LightManager::GetSpotLight(const std::string& name_)
{
	for (auto& light : spotLights_) {
		if (light->name_ == name_) {
			return light.get();
		}
	}
	return nullptr;
}

void LightManager::CreateLightResource()
{
	// 必要なら Resize
	size_t dirSize = std::max<size_t>(1, maxDirLights);
	dxManager_->CreateBufferResource(sizeof(DirectionalLightData) * dirSize, dirLightResource_);

	size_t pointSize = std::max<size_t>(1, maxPointLights);
	dxManager_->CreateBufferResource(sizeof(PointLightData) * pointSize, pointLightResource_);

	size_t spotSize = std::max<size_t>(1, maxSpotLights);
	dxManager_->CreateBufferResource(sizeof(SpotLightData) * spotSize, spotLightResource_);
}

void LightManager::UpdateBuffer(ID3D12Resource* resource, const void* data, size_t size)
{
	void* mapped = nullptr;
	resource->Map(0, nullptr, &mapped);
	memcpy(mapped, data, size);
	resource->Unmap(0, nullptr);
}
