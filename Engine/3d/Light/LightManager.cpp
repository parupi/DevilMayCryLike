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

	CreateDummyLightResources();
}

void LightManager::Finalize()
{
	DeleteAllLight();

	if (aggregatedDirBuffer_) aggregatedDirBuffer_.Reset();
	if (aggregatedPointBuffer_) aggregatedPointBuffer_.Reset();
	if (aggregatedSpotBuffer_) aggregatedSpotBuffer_.Reset();

	if (dummyDirLight_) dummyDirLight_.Reset();
	if (dummyPointLight_) dummyPointLight_.Reset();
	if (dummySpotLight_) dummySpotLight_.Reset();

	dxManager_ = nullptr;

	if (instance) {
		delete instance;
		instance = nullptr;
	}
}

void LightManager::BindLightsToShader()
{
	auto commandList = dxManager_->GetCommandList();

	// Directional: aggregated があればそれを、なければダミーを渡す
	if (aggregatedDirBuffer_) {
		commandList->SetGraphicsRootConstantBufferView(4, aggregatedDirBuffer_->GetGPUVirtualAddress());
	} else {
		commandList->SetGraphicsRootConstantBufferView(4, dummyDirLight_->GetGPUVirtualAddress());
	}

	// Point
	if (aggregatedPointBuffer_) {
		commandList->SetGraphicsRootConstantBufferView(5, aggregatedPointBuffer_->GetGPUVirtualAddress());
	} else {
		commandList->SetGraphicsRootConstantBufferView(5, dummyPointLight_->GetGPUVirtualAddress());
	}

	// Spot
	if (aggregatedSpotBuffer_) {
		commandList->SetGraphicsRootConstantBufferView(6, aggregatedSpotBuffer_->GetGPUVirtualAddress());
	} else {
		commandList->SetGraphicsRootConstantBufferView(6, dummySpotLight_->GetGPUVirtualAddress());
	}

	
}

void LightManager::BindLightsForDeferred()
{
	auto commandList = dxManager_->GetCommandList();

	commandList->SetGraphicsRootConstantBufferView(1, aggregatedDirBuffer_->GetGPUVirtualAddress());
}

void LightManager::UpdateAllLight()
{
	// 各種ライトの更新
	for (auto& dir : dirLights_) { 
		dir->UpdateLightResource(); 
		dir->Update();
	}
	for (auto& point : pointLights_) { 
		point->UpdateLightResource(); 
		point->Update();
	}
	for (auto& spot : spotLights_){
		spot->UpdateLightResource();
		spot->Update();
	}

	// シェーダに渡す配列を作る
	UpdateAggregatedBuffers();
}

DirectionalLight* LightManager::CreateDirectionalLight(const std::string& name)
{
	auto light = std::make_unique<DirectionalLight>(name);
	light->Initialize(dxManager_);
	DirectionalLight* ptr = light.get();
	dirLights_.push_back(std::move(light));
	return ptr;
}

PointLight* LightManager::CreatePointLight(const std::string& name)
{
	auto light = std::make_unique<PointLight>(name);
	light->Initialize(dxManager_);
	PointLight* ptr = light.get();
	pointLights_.push_back(std::move(light));
	return ptr;
}

SpotLight* LightManager::CreateSpotLight(const std::string& name)
{
	auto light = std::make_unique<SpotLight>(name);
	light->Initialize(dxManager_);
	SpotLight* ptr = light.get();
	spotLights_.push_back(std::move(light));
	return ptr;
}

void LightManager::CreateDummyLightResources()
{
	DirectionalLightData dir{};
	PointLightData point{};
	SpotLightData spot{};

	// 各要素サイズを確保（常にCBは256バイト境界で確保しておく）
	size_t dirAlloc = Align256(sizeof(DirectionalLightData));
	size_t pointAlloc = Align256(sizeof(PointLightData));
	size_t spotAlloc = Align256(sizeof(SpotLightData));

	// 最低でも 256 バイトを確保する（Align256 により既に満たされるが明示）
	dirAlloc = std::max<size_t>(dirAlloc, 256);
	pointAlloc = std::max<size_t>(pointAlloc, 256);
	spotAlloc = std::max<size_t>(spotAlloc, 256);

	dxManager_->CreateBufferResource(dirAlloc, dummyDirLight_);
	dxManager_->CreateBufferResource(pointAlloc, dummyPointLight_);
	dxManager_->CreateBufferResource(spotAlloc, dummySpotLight_);

	// CPU に書き込む（先頭に配置）
	UpdateBuffer(dummyDirLight_.Get(), &dir, sizeof(DirectionalLightData));
	UpdateBuffer(dummyPointLight_.Get(), &point, sizeof(PointLightData));
	UpdateBuffer(dummySpotLight_.Get(), &spot, sizeof(SpotLightData));
}

DirectionalLight* LightManager::GetDirectionalLight(const std::string& name_)
{
	for (auto& light : dirLights_) {
		if (light->GetName() == name_) {
			return light.get();
		}
	}
	return nullptr;
}

PointLight* LightManager::GetPointLight(const std::string& name_)
{
	for (auto& light : pointLights_) {
		if (light->GetName() == name_) {
			return light.get();
		}
	}
	return nullptr;
}

SpotLight* LightManager::GetSpotLight(const std::string& name_)
{
	for (auto& light : spotLights_) {
		if (light->GetName() == name_) {
			return light.get();
		}
	}
	return nullptr;
}

void LightManager::DeleteAllLight()
{
	// CPU 側のライトデータ削除
	dirLights_.clear();
	pointLights_.clear();
	spotLights_.clear();
}

void LightManager::CreateAggregatedBuffers()
{
	// Directional
	size_t dirCount = dirLights_.size();
	if (dirCount > 0) {
		size_t dirSize = Align256(dirCount * sizeof(DirectionalLightData));
		// 再作成は必要なときだけ（既に同じサイズなら再作成しない方がよい）
		bool recreate = true;
		if (aggregatedDirBuffer_) {
			auto desc = aggregatedDirBuffer_->GetDesc();
			if (desc.Width >= dirSize) recreate = false; // 既に十分なら作り直さない
			else aggregatedDirBuffer_.Reset();
		}
		if (recreate) dxManager_->CreateBufferResource(dirSize, aggregatedDirBuffer_);
	} else {
		// 0 のときは aggregated をリセット（ダミーが使われる）
		aggregatedDirBuffer_.Reset();
	}

	// Point
	size_t pointCount = pointLights_.size();
	if (pointCount > 0) {
		size_t pointSize = Align256(pointCount * sizeof(PointLightData));
		bool recreate = true;
		if (aggregatedPointBuffer_) {
			auto desc = aggregatedPointBuffer_->GetDesc();
			if (desc.Width >= pointSize) recreate = false;
			else aggregatedPointBuffer_.Reset();
		}
		if (recreate) dxManager_->CreateBufferResource(pointSize, aggregatedPointBuffer_);
	} else {
		aggregatedPointBuffer_.Reset();
	}

	// Spot
	size_t spotCount = spotLights_.size();
	if (spotCount > 0) {
		size_t spotSize = Align256(spotCount * sizeof(SpotLightData));
		bool recreate = true;
		if (aggregatedSpotBuffer_) {
			auto desc = aggregatedSpotBuffer_->GetDesc();
			if (desc.Width >= spotSize) recreate = false;
			else aggregatedSpotBuffer_.Reset();
		}
		if (recreate) dxManager_->CreateBufferResource(spotSize, aggregatedSpotBuffer_);
	} else {
		aggregatedSpotBuffer_.Reset();
	}
}

void LightManager::UpdateAggregatedBuffers()
{
	// ここで生成をしておく
	CreateAggregatedBuffers();

	// CPU側を合わせる
	if (dirLights_.size() > 0 && aggregatedDirBuffer_) {
		std::vector<DirectionalLightData> v;
		v.reserve(dirLights_.size());
		for (auto& l : dirLights_) {
			// シリアライズ
			DirectionalLightData tmp;
			l->SerializeTo(&tmp);
			v.push_back(tmp);
		}
		// upload
		void* mapped = nullptr;
		aggregatedDirBuffer_->Map(0, nullptr, &mapped);
		std::memcpy(mapped, v.data(), sizeof(DirectionalLightData) * v.size());
		aggregatedDirBuffer_->Unmap(0, nullptr);
	}

	// 他のライトにも同じ処理をする
	if (pointLights_.size() > 0 && aggregatedPointBuffer_) {
		std::vector<PointLightData> v;
		v.reserve(pointLights_.size());
		for (auto& l : pointLights_) {
			PointLightData tmp;
			l->SerializeTo(&tmp);
			v.push_back(tmp);
		}
		void* mapped = nullptr;
		aggregatedPointBuffer_->Map(0, nullptr, &mapped);
		std::memcpy(mapped, v.data(), sizeof(PointLightData) * v.size());
		aggregatedPointBuffer_->Unmap(0, nullptr);
	}

	if (spotLights_.size() > 0 && aggregatedSpotBuffer_) {
		std::vector<SpotLightData> v;
		v.reserve(spotLights_.size());
		for (auto& l : spotLights_) {
			SpotLightData tmp;
			l->SerializeTo(&tmp);
			v.push_back(tmp);
		}
		void* mapped = nullptr;
		aggregatedSpotBuffer_->Map(0, nullptr, &mapped);
		std::memcpy(mapped, v.data(), sizeof(SpotLightData) * v.size());
		aggregatedSpotBuffer_->Unmap(0, nullptr);
	}
}

void LightManager::UpdateBuffer(ID3D12Resource* resource, const void* data, size_t size)
{
	assert(resource && data && size > 0);
	void* mapped = nullptr;
	resource->Map(0, nullptr, &mapped);
	memcpy(mapped, data, size);
	resource->Unmap(0, nullptr);
}

inline size_t Align256(size_t s)
{
	return (s + 255u) & ~255u;
}
