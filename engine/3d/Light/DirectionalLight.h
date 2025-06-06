#pragma once
#include "BaseLight.h"
#include "LightStructs.h"
class DirectionalLight : public BaseLight
{
public:
	DirectionalLight(std::string lightName);

	// 更新処理
	void Update() override;

	// アクセッサ
	DirectionalLightData& GetLightData() { return lightData_; }
private:

	DirectionalLightData lightData_;
};
