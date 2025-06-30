#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(std::string lightName)
	: BaseLight()
{
	name_ = lightName;
}


void DirectionalLight::Update()
{
}
