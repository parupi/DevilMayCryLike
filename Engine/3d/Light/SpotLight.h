#pragma once
#include "BaseLight.h"
#include "LightStructs.h"
class SpotLight : public BaseLight
{
public:
	SpotLight(std::string lightName);

	void Update() override;

	SpotLightData& GetLightData() { return lightData_; }

private:
	SpotLightData lightData_;
};

