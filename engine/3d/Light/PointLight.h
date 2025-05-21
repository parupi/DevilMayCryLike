#pragma once
#include "BaseLight.h"
#include "LightStructs.h"
class PointLight : BaseLight
{
public:
	PointLight(std::string lightName);

	// 更新処理
	void Update() override;

	// アクセッサ
	PointLightData& GetLightData() { return lightData_; }

	std::string GetName() const { return name; }
private:
	PointLightData lightData_;
};

