#pragma once
#include <vector>
#include <math/Vector4.h>
#include <math/Vector3.h>
#include <wrl.h>
#include <d3d12.h>
#include <mutex>
#include "BaseLight.h"
#include "LightStructs.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include <base/DirectXManager.h>

constexpr int maxDirLights = 3;
constexpr int maxPointLights = 3; // ポイントライトの最大数
constexpr int maxSpotLights = 3;   // スポットライトの最大数

class LightManager
{
private:
	static LightManager* instance;
	static std::once_flag initInstanceFlag;

	LightManager() = default;
	~LightManager() = default;
	LightManager(LightManager&) = default;
	LightManager& operator=(LightManager&) = default;
public:
	// シングルトンインスタンスの取得
	static LightManager* GetInstance();

	void Initialize(DirectXManager* dxManager);
	void Finalize();
	// 描画前処理
	void BindLightsToShader();
	// ライトの更新処理
	void UpdateAllLight();
	// ライトの追加
	void AddDirectionalLight(std::unique_ptr<DirectionalLight> light);
	void AddPointLight(std::unique_ptr<PointLight> light);
	void AddSpotLight(std::unique_ptr<SpotLight> light);

	DirectionalLight* GetDirectionalLight(const std::string& name_);
	PointLight* GetPointLight(const std::string& name_);
	SpotLight* GetSpotLight(const std::string& name_);

	void DeleteAllLight();
private:
	//void CreateDirLightResource();
	//void CreatePointLightResource();
	//void CreateSpotLightResource();

	void CreateLightResource();
	void UpdateBuffer(ID3D12Resource* resource, const void* data, size_t size);
private:
	DirectXManager* dxManager_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> dirLightResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_ = nullptr;

	std::vector<std::unique_ptr<DirectionalLight>> dirLights_;
	std::vector< std::unique_ptr<PointLight>> pointLights_;
	std::vector<std::unique_ptr<SpotLight>> spotLights_;

};

