#include "SpotLight.h"
#ifdef _DEBUG
#include <imgui.h>
#include "3d/Primitive/PrimitiveLineDrawer.h"
#endif // DEBUG
#include <numbers>
#include "math/function.h"

SpotLight::SpotLight(const std::string& name)
{
	name_ = name;
	// ライトの情報を初期化
	lightData_ = {};
	lightData_.type = static_cast<int>(LightType::Spot);
	lightData_.enabled = 1;
	lightData_.intensity = 1.0f;
	lightData_.decay = 1.0f;
	lightData_.color = { 1,1,1,1 };
	lightData_.position = { 0.0f, 0.0f, 0.0f };
	lightData_.direction = { 0.0f, 1.0f, 0.0f };
	lightData_.cosAngle = 0.9f;
	lightData_.distance = 20.0f;

	Initialize();
}

void SpotLight::Initialize()
{
	global_ = GlobalVariables::GetInstance();
	// 既存のライトデータを読み込み
	global_->LoadFile("Light", name_);
	// エディター項目登録
	global_->CreateGroup(name_);
	global_->AddItem(name_, "Color", Vector4{ 1, 1, 1, 1 });
	global_->AddItem(name_, "Position", Vector3{ 0, 0, 0 });
	global_->AddItem(name_, "Direction", Vector3{ 0, -1, 0 });
	global_->AddItem(name_, "Intensity", 1.0f);
	global_->AddItem(name_, "Distance", 20.0f);
	global_->AddItem(name_, "Decay", 1.0f);
	global_->AddItem(name_, "CosAngle", 0.9f); // 角度の余弦値（0.9 ≒ 約25°）
	global_->AddItem(name_, "Enabled", true);
}

void SpotLight::Update()
{
	lightData_.enabled = global_->GetValueRef<bool>(name_, "Enabled");
	lightData_.color = global_->GetValueRef<Vector4>(name_, "Color");
	lightData_.position = global_->GetValueRef<Vector3>(name_, "Position");
	Vector3 dir = global_->GetValueRef<Vector3>(name_, "Direction");
	lightData_.direction = dir;
	lightData_.intensity = global_->GetValueRef<float>(name_, "Intensity");
	lightData_.distance = global_->GetValueRef<float>(name_, "Distance");
	lightData_.decay = global_->GetValueRef<float>(name_, "Decay");
	lightData_.cosAngle = global_->GetValueRef<float>(name_, "CosAngle");
}

#ifdef _DEBUG
void SpotLight::DrawLightEditor()
{
	bool& enabled = global_->GetValueRef<bool>(name_, "Enabled");
	ImGui::Checkbox("Enabled", &enabled);

	Vector4& color = global_->GetValueRef<Vector4>(name_, "Color");
	ImGui::ColorEdit4("Color", &color.x);

	Vector3& position = global_->GetValueRef<Vector3>(name_, "Position");
	ImGui::DragFloat3("Position", &position.x, 0.1f);

	Vector3& direction = global_->GetValueRef<Vector3>(name_, "Direction");
	if (ImGui::DragFloat3("Direction", &direction.x, 0.01f, -1.0f, 1.0f)) {
		// 変更された瞬間に正規化
		if (Length(direction) > 0.0001f) {
			direction = Normalize(direction);
		}
	}

	float& intensity = global_->GetValueRef<float>(name_, "Intensity");
	ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);

	float& distance = global_->GetValueRef<float>(name_, "Distance");
	ImGui::DragFloat("Distance", &distance, 0.1f, 0.0f, 1000.0f);

	float& decay = global_->GetValueRef<float>(name_, "Decay");
	ImGui::DragFloat("Decay", &decay, 0.01f, 0.0f, 10.0f);

	float& cosAngle = global_->GetValueRef<float>(name_, "CosAngle");
	ImGui::SliderFloat("Cos Angle", &cosAngle, 0.0f, 1.0f);

	if (ImGui::Button("Save##SpotLight")) {
		global_->SaveFile("Light", name_);
	}
}

void SpotLight::DrawDebug(PrimitiveLineDrawer* drawer)
{
	if (!lightData_.enabled) return;

	Vector3 apex = lightData_.position;

	Vector3 dir = lightData_.direction;
	if (Length(dir) < 0.0001f) return;
	dir = Normalize(dir);

	float distance = lightData_.distance;
	float cosAngle = lightData_.cosAngle;

	if (distance <= 0.0f || cosAngle <= 0.0f) return;

	// ---- 半径計算 ----
	float sinAngle = std::sqrt(1.0f - cosAngle * cosAngle);
	float radius = distance * sinAngle / cosAngle;

	Vector3 baseCenter = apex + dir * distance;

	Vector4 color = lightData_.color;
	color.w = 1.0f;

	const int divide = 24;

	// 中心線
	drawer->DrawLine(apex, baseCenter, color);

	// 底面円
	drawer->DrawWireCircle(baseCenter, radius, dir, color, divide);

	// 側面ライン
	// 底面円の直交基底を作る
	Vector3 tangent;
	if (std::abs(dir.y) < 0.99f) {
		tangent = Normalize(Cross(dir, { 0,1,0 }));
	} else {
		tangent = Normalize(Cross(dir, { 1,0,0 }));
	}
	
	Vector3 bitangent = Cross(dir, tangent);

	const float angleStep = 2.0f * std::numbers::pi_v<float> / divide;

	for (int i = 0; i < divide; ++i)
	{
		float angle = i * angleStep;

		Vector3 circlePoint = baseCenter + tangent * std::cos(angle) * radius + bitangent * std::sin(angle) * radius;

		drawer->DrawLine(apex, circlePoint, color);
	}
}
#endif // DEBUG