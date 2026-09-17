#include "Prop.h"
#include <memory>
#include "Math/MathUtils.h"
#include "World3D/Object/Renderer/RendererManager.h"
#include "World3D/Object/Renderer/ModelRenderer.h"
#include "World3D/Object/Model/ModelManager.h"
#include "World3D/Light/LightManager.h"
#include "World3D/Light/DynamicPointLight.h"

Prop::Prop(std::string objectName) : Object3d(objectName) {
	Object3d::Initialize();
}

Prop::~Prop() {
	// シーン終了時は DeleteAllLight で先に消えていることもある（その場合は何も起きない）
	LightManager::GetInstance().RemoveLight(light_);
}

void Prop::Initialize() {
	// モデル未指定なら Cube。ここで確定させておけば保存にもそのまま出る
	if (GetModelName().empty()) {
		SetModelName("Cube");
	}

	// ステージデータで指定されたモデルを未読み込みなら読み込む
	ModelManager::GetInstance().LoadModel(GetModelName());

	// レンダラーの生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_, GetModelName()));
	AddRenderer(RendererManager::GetInstance().FindRender(name_));

	// コライダーはステージデータで付けた場合のみ存在する。
	// 付いていればGroundカテゴリにして、プレイヤーが通り抜けないようにする
	for (BaseCollider* collider : GetColliders()) {
		collider->category_ = CollisionCategory::Ground;
	}

	// ランタンなど発光する小物用のポイントライト
	if (hasLight_) {
		SetLightEnabled(true);
	}
}

void Prop::SetLightEnabled(bool enabled) {
	hasLight_ = enabled;

	if (!enabled) {
		LightManager::GetInstance().RemoveLight(light_);
		light_ = nullptr;
		return;
	}
	if (light_) {
		ApplyLightParams();
		return;
	}

	auto light = std::make_unique<DynamicPointLight>(name_ + "Light");
	light_ = static_cast<DynamicPointLight*>(LightManager::GetInstance().AddLight(std::move(light)));
	ApplyLightParams();
	// 位置は Update() が毎フレーム入れるが、matWorld_ はまだ組まれていないので初回だけ直接置く
	light_->SetPosition(GetWorldTransform()->GetTranslation() + lightOffset_);
}

void Prop::ApplyLightParams() {
	if (!light_) {
		return;
	}
	// 位置は Update() がトランスフォームから毎フレーム入れるのでここでは触らない
	light_->SetColor({ lightColor_.x, lightColor_.y, lightColor_.z, 1.0f });
	light_->SetIntensity(lightIntensity_);
	light_->SetRadius(lightRadius_);
	light_->SetDecay(lightDecay_);
}

void Prop::Update(float deltaTime) {
	Object3d::Update(deltaTime);

	// ライトをオブジェクトに追従させる（オフセットは回転・スケールにも追従する）
	if (light_) {
		light_->SetPosition(Transform(lightOffset_, GetWorldTransform()->GetMatWorld()));
	}
}

