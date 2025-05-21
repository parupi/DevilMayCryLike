#pragma once
#include "BaseLight.h"
#include "LightStructs.h"
class DirectionalLight : BaseLight
{
public:
	DirectionalLight(std::string lightName);

	// 更新処理
	void Update() override;

	// アクセッサ
	DirectionalLightData& GetLightData() { return lightData_; }

	std::string GetName() const { return name; }
private:

	DirectionalLightData lightData_;
};
