#pragma once
#include "BaseLight.h"
#include "LightStructs.h"
class PointLight : public BaseLight
{
public:
	PointLight(std::string lightName);

	// 更新処理
	void Update() override;

	// アクセッサ
	PointLightData& GetLightData() { return lightData_; }
private:
	PointLightData lightData_;
};

