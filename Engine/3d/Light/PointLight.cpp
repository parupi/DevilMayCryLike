#include "PointLight.h"
#ifdef _DEBUG
#include <imgui.h>
#include "3d/Primitive/PrimitiveLineDrawer.h"
#endif // DEBUG

PointLight::PointLight(const std::string& name)
{
	name_ = name;
	// ライトの情報を初期化
	lightData_ = {};
	lightData_.type = static_cast<int>(LightType::Point);
	lightData_.enabled = 1;
	lightData_.intensity = 1.0f;
	lightData_.decay = 1.0f;
	lightData_.color = { 1,1,1,1 };
	lightData_.position = { 0.0f, 0.0f, 0.0f };
	lightData_.radius = 10.0f;

	Initialize();
}

void PointLight::Initialize()
{
	global_ = GlobalVariables::GetInstance();
	// 既存のライトデータを読み込み
	global_->LoadFile("Light", name_);
	// 各種パラメータをエディターに追加
	global_->AddItem(name_, "Color", Vector4{ 1,1,1,1 });
	global_->AddItem(name_, "Position", Vector3{ 0,0,0 });
	global_->AddItem(name_, "Intensity", 1.0f);
	global_->AddItem(name_, "Radius", 10.0f);
	global_->AddItem(name_, "Decay", 1.0f);
	global_->AddItem(name_, "Enabled", true);
}

void PointLight::Update()
{
	lightData_.enabled = global_->GetValueRef<bool>(name_, "Enabled");
	lightData_.color = global_->GetValueRef<Vector4>(name_, "Color");
	lightData_.position = global_->GetValueRef<Vector3>(name_, "Position");
	lightData_.intensity = global_->GetValueRef<float>(name_, "Intensity");
	lightData_.radius = global_->GetValueRef<float>(name_, "Radius");
	lightData_.decay = global_->GetValueRef<float>(name_, "Decay");
}

#ifdef _DEBUG
void PointLight::DrawLightEditor()
{
	// ====== Enabled ======
	bool& enabled = global_->GetValueRef<bool>(name_, "Enabled");
	ImGui::Checkbox("Enabled", &enabled);

	// ====== Color ======
	Vector4& color = global_->GetValueRef<Vector4>(name_, "Color");
	ImGui::ColorEdit4("Color", &color.x);

	// ====== Position ======
	Vector3& position = global_->GetValueRef<Vector3>(name_, "Position");
	ImGui::DragFloat3("Position", &position.x, 0.1f);

	// ====== Intensity ======
	float& intensity = global_->GetValueRef<float>(name_, "Intensity");
	ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);

	// ====== Radius ======
	float& radius = global_->GetValueRef<float>(name_, "Radius");
	ImGui::DragFloat("Radius", &radius, 0.1f, 0.0f, 1000.0f);

	// ====== Decay ======
	float& decay = global_->GetValueRef<float>(name_, "Decay");
	ImGui::DragFloat("Decay", &decay, 0.01f, 0.0f, 10.0f);

	// ====== セーブ & ロード ======
	if (ImGui::Button("Save##Light")) {
		global_->SaveFile("Light", name_);
	}
}

void PointLight::DrawDebug(PrimitiveLineDrawer* drawer)
{
	drawer->DrawWireSphere(lightData_.position, lightData_.radius, lightData_.color);
}
#endif // DEBUG