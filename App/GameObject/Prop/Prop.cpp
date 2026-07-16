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
	// レベルエディタ(file_name)で指定されたモデルを未読み込みなら読み込む
	ModelManager::GetInstance().LoadModel(modelName_);

	// レンダラーの生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_, modelName_));
	AddRenderer(RendererManager::GetInstance().FindRender(name_));

	// コライダーはレベルエディタで付けた場合のみ存在する。
	// 付いていればGroundカテゴリにして、プレイヤーが通り抜けないようにする
	if (auto* collider = GetCollider(name_)) {
		collider->category_ = CollisionCategory::Ground;
	}

	// ランタンなど発光する小物用のポイントライト
	if (hasLight_) {
		auto light = std::make_unique<DynamicPointLight>(name_ + "Light");
		light->SetColor({ lightColor_.x, lightColor_.y, lightColor_.z, 1.0f });
		light->SetIntensity(lightIntensity_);
		light->SetRadius(lightRadius_);
		light->SetDecay(lightDecay_);
		light->SetPosition(GetWorldTransform()->GetTranslation() + lightOffset_);

		light_ = static_cast<DynamicPointLight*>(LightManager::GetInstance().AddLight(std::move(light)));
	}
}

void Prop::Update(float deltaTime) {
	Object3d::Update(deltaTime);

	// ライトをオブジェクトに追従させる（オフセットは回転・スケールにも追従する）
	if (light_) {
		light_->SetPosition(Transform(lightOffset_, GetWorldTransform()->GetMatWorld()));
	}
}

#ifdef _DEBUG
void Prop::DebugGui() {
	ImGui::Begin("Prop");
	Object3d::DebugGui();
	ImGui::End();
}
#endif // _DEBUG
