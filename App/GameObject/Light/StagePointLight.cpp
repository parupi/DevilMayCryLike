#include "StagePointLight.h"
#include "Math/MathUtils.h"
#include "World3D/Light/LightManager.h"
#include "World3D/Light/DynamicPointLight.h"

StagePointLight::StagePointLight(std::string objectName) : Object3d(objectName) {
	Object3d::Initialize();
}

StagePointLight::~StagePointLight() {
	// シーン終了時は DeleteAllLight で先に消えていることもある（その場合は何も起きない）
	LightManager::GetInstance().RemoveLight(light_);
}

void StagePointLight::Initialize() {
	auto light = std::make_unique<DynamicPointLight>(name_);
	light->SetColor({ color_.x, color_.y, color_.z, 1.0f });
	light->SetIntensity(intensity_);
	light->SetRadius(radius_);
	light->SetDecay(decay_);
	light->SetPosition(GetWorldTransform()->GetTranslation() + offset_);

	light_ = static_cast<DynamicPointLight*>(LightManager::GetInstance().AddLight(std::move(light)));
}

void StagePointLight::Update(float deltaTime) {
	Object3d::Update(deltaTime);

	// トランスフォームに追従させる（デバッグ中に動かしても反映される）
	if (light_) {
		light_->SetPosition(Transform(offset_, GetWorldTransform()->GetMatWorld()));
	}
}
