#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(std::string lightName) : BaseLight(lightName)
{
	name_ = lightName;
}


void DirectionalLight::Update()
{
}

void DirectionalLight::DrawLightEditor()
{
}

void DirectionalLight::CreateLightResource()
{
}

void DirectionalLight::UpdateLightResource()
{
	auto data = lightData_;
	void* mapped = nullptr;
	resource_->Map(0, nullptr, &mapped);
	memcpy(mapped, &data, sizeof(DirectionalLightData));
	resource_->Unmap(0, nullptr);
}

void DirectionalLight::SerializeTo(void* dest) const
{
	dest;
}
