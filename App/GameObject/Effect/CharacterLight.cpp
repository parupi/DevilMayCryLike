#include "CharacterLight.h"
#include <memory>
#include "World3D/Light/LightManager.h"
#include "World3D/Light/DynamicPointLight.h"

CharacterLight::~CharacterLight() {
	// シーン終了時は DeleteAllLight で先に消えていることもある（その場合は何も起きない）
	LightManager::GetInstance().RemoveLight(light_);
}

void CharacterLight::Initialize(const std::string& name, const Vector4& color) {
	baseColor_ = color;

	auto light = std::make_unique<DynamicPointLight>(name);
	light->SetColor(color);
	light->SetIntensity(baseIntensity_);
	light->SetRadius(baseRadius_);

	light_ = static_cast<DynamicPointLight*>(LightManager::GetInstance().AddLight(std::move(light)));
}

void CharacterLight::Update(const Vector3& ownerPosition, float deltaTime) {
	if (!light_) return;

	if (flashTimer_ > 0.0f) {
		flashTimer_ -= deltaTime;
		if (flashTimer_ < 0.0f) flashTimer_ = 0.0f;
	}

	// フラッシュの残り割合(1→0)を2乗で減衰させ、「パッと光ってスッと戻る」カーブにする
	float k = flashTimer_ / kFlashDuration;
	k *= k;

	light_->SetPosition(ownerPosition + Vector3{ 0.0f, heightOffset_, 0.0f });
	light_->SetIntensity(baseIntensity_ * intensityScale_ * (1.0f + (kFlashIntensityScale - 1.0f) * k));
	light_->SetRadius(baseRadius_ * (1.0f + (kFlashRadiusScale - 1.0f) * k));
}

void CharacterLight::Flash() {
	flashTimer_ = kFlashDuration;
}

void CharacterLight::SetEnabled(bool enabled) {
	if (light_) {
		light_->SetEnabled(enabled);
	}
}

void CharacterLight::SetColorOverride(const Vector4& color) {
	if (light_) {
		light_->SetColor(color);
	}
}

void CharacterLight::ClearColorOverride() {
	if (light_) {
		light_->SetColor(baseColor_);
	}
}
