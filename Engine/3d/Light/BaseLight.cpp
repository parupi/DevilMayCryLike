#include "BaseLight.h"


BaseLight::BaseLight(const std::string& name)
{
	name_ = name;
}

BaseLight::~BaseLight()
{
	resource_.Reset();
}

void BaseLight::Initialize(DirectXManager* dxManager)
{
	dxManager_ = dxManager;
	CreateLightResource();
	isDirty_ = true;
}
