#include "StageSpotLight.h"
#include "Math/MathUtils.h"
#include "World3D/Light/LightManager.h"
#include "World3D/Light/DynamicSpotLight.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kDegToRad = 3.14159265f / 180.0f;
}

StageSpotLight::StageSpotLight(std::string objectName) : Object3d(objectName) {
	Object3d::Initialize();
}

StageSpotLight::~StageSpotLight() {
	// シーン終了時は DeleteAllLight で先に消えていることもある（その場合は何も起きない）
	LightManager::GetInstance().RemoveLight(light_);
}

void StageSpotLight::Initialize() {
	auto light = std::make_unique<DynamicSpotLight>(name_);
	light_ = static_cast<DynamicSpotLight*>(LightManager::GetInstance().AddLight(std::move(light)));
	ApplyLightParams();
	// 位置と向きは Update() が毎フレーム入れるが、matWorld_ はまだ組まれていないので初回だけ直接置く
	light_->SetPosition(GetWorldTransform()->GetTranslation());
	light_->SetDirection({ 0.0f, -1.0f, 0.0f });
}

void StageSpotLight::ApplyLightParams() {
	if (!light_) {
		return;
	}
	light_->SetColor({ params_.color.x, params_.color.y, params_.color.z, 1.0f });
	light_->SetIntensity(params_.intensity);
	light_->SetDistance(params_.distance);
	light_->SetDecay(params_.decay);
	const float outer = std::clamp(params_.outerAngle, 0.5f, 89.0f);
	const float inner = std::clamp(params_.innerAngle, 0.0f, outer - 0.1f);
	light_->SetCosAngle(std::cos(outer * kDegToRad));
	light_->SetSpotInnerCos(std::cos(inner * kDegToRad));
	light_->SetParticleLightingScale((std::max)(params_.particleLighting, 0.0f));
	light_->SetVolumetricScale((std::max)(params_.volumetric, 0.0f));
}

Vector3 StageSpotLight::GetWorldDirection() {
	const Matrix4x4& m = GetWorldTransform()->GetMatWorld();
	// 行ベクトルの規約なので、ローカルの -Y は2行目を反転したもの
	Vector3 dir{ -m.m[1][0], -m.m[1][1], -m.m[1][2] };
	const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
	return len > 1e-5f ? Vector3{ dir.x / len, dir.y / len, dir.z / len } : Vector3{ 0.0f, -1.0f, 0.0f };
}

Vector3 StageSpotLight::GetWorldPosition() {
	const Matrix4x4& m = GetWorldTransform()->GetMatWorld();
	return { m.m[3][0], m.m[3][1], m.m[3][2] };
}

void StageSpotLight::Update(float deltaTime) {
	Object3d::Update(deltaTime);

	// トランスフォームに追従させる（デバッグ中に動かしたり回したりしても反映される）
	if (light_) {
		light_->SetPosition(GetWorldPosition());
		light_->SetDirection(GetWorldDirection());
	}
}
