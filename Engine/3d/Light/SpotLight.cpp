#include "SpotLight.h"

SpotLight::SpotLight(std::string lightName) : BaseLight(lightName)
{
	name_ = lightName;
}

void SpotLight::Update()
{
}

void SpotLight::DrawLightEditor()
{
}

void SpotLight::CreateLightResource()
{
}

void SpotLight::UpdateLightResource()
{
    auto data = lightData_;
    void* mapped = nullptr;
    resource_->Map(0, nullptr, &mapped);
    memcpy(mapped, &data, sizeof(SpotLightData));
    resource_->Unmap(0, nullptr);
}

void SpotLight::SerializeTo(void* dest) const
{
	dest;
}
