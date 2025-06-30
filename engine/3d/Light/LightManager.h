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

public: // ゲッター // セッター //
	//// DirLight
	//Vector4 GetDirLightColor(uint32_t index) const;
	//void SetDirLightColor(uint32_t index, const Vector4& color);
	//Vector3 GetDirLightDirection(uint32_t index) const;
	//void SetDirLightDirection(uint32_t index, const Vector3& dir);
	//float GetDirLightIntensity(uint32_t index) const;
	//void SetDirLightIntensity(uint32_t index, float intensity);
	//bool GetDirLightActive(uint32_t index) const;
	//void SetDirLightActive(uint32_t index, bool isActive);
	//// PointLight
	//Vector4 GetPointLightColor(uint32_t index) const;
	//void SetPointLightColor(uint32_t index, const Vector4& color);
	//Vector3 GetPointLightPos(uint32_t index) const;
	//void SetPointLightPos(uint32_t index, const Vector3& pos);
	//float GetPointLightIntensity(uint32_t index) const;
	//void SetPointLightIntensity(uint32_t index, float intensity);
	//float GetPointLightRadius(uint32_t index) const;
	//void SetPointLightRadius(uint32_t index, float radius);
	//void SetPointLightDecay(uint32_t index, float decay);
	//float GetPointLightDecay(uint32_t index) const;
	//bool GetPointLightActive(uint32_t index) const;
	//void SetPointLightActive(uint32_t index, bool isActive);
	//// SpotLight
	//void SetSpotLightColor(uint32_t index, const Vector4& color);
	//void SetSpotLightPos(uint32_t index, const Vector3& pos);
	//void SetSpotLightIntensity(uint32_t index, float intensity);
	//void SetSpotLightDirection(uint32_t index, const Vector3& dir);
	//void SetSpotLightDistance(uint32_t index, float distance);
	//void SetSpotLightDecay(uint32_t index, float decay);
	//void SetSpotLightCosAngle(uint32_t index, float cosAngle);
	//void SetSpotLightActive(uint32_t index, bool isActive);
	//Vector4 GetSpotLightColor(uint32_t index) const;
	//Vector3 GetSpotLightPos(uint32_t index) const;
	//float GetSpotLightIntensity(uint32_t index) const;
	//Vector3 GetSpotLightDirection(uint32_t index) const;
	//float GetSpotLightDistance(uint32_t index) const;
	//float GetSpotLightDecay(uint32_t index) const;
	//float GetSpotLightCosAngle(uint32_t index) const;
	//bool GetSpotLightActive(uint32_t index) const;
};

