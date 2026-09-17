#include "DynamicSpotLight.h"
#include <cmath>
#include "Math/MathUtils.h"
#ifdef _DEBUG
#include <imgui.h>
#include "World3D/Primitive/PrimitiveLineDrawer.h"
#endif // DEBUG

DynamicSpotLight::DynamicSpotLight(const std::string& name)
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
	lightData_.direction = { 0.0f, -1.0f, 0.0f };
	lightData_.cosAngle = 0.9f;
	lightData_.distance = 20.0f;

	Initialize();
}

void DynamicSpotLight::Initialize()
{
}

void DynamicSpotLight::Update()
{
	// 所有者が Setter で毎フレーム更新するため何もしない
}

void DynamicSpotLight::SetDirection(const Vector3& direction)
{
	if (Length(direction) > 0.0001f) {
		lightData_.direction = Normalize(direction);
	}
}

#ifdef _DEBUG
void DynamicSpotLight::DrawLightEditor()
{
	ImGui::TextUnformatted("Runtime-driven light (owner overwrites values every frame)");

	bool enabled = lightData_.enabled != 0;
	if (ImGui::Checkbox("Enabled", &enabled)) {
		lightData_.enabled = enabled ? 1 : 0;
	}
	ImGui::ColorEdit4("Color", &lightData_.color.x);
	ImGui::DragFloat3("Position", &lightData_.position.x, 0.1f);
	ImGui::DragFloat3("Direction", &lightData_.direction.x, 0.01f, -1.0f, 1.0f);
	ImGui::DragFloat("Intensity", &lightData_.intensity, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("Distance", &lightData_.distance, 0.1f, 0.0f, 1000.0f);
	ImGui::DragFloat("Decay", &lightData_.decay, 0.01f, 0.0f, 10.0f);
	ImGui::SliderFloat("Cos Angle", &lightData_.cosAngle, 0.0f, 1.0f);
}

void DynamicSpotLight::DrawDebug(PrimitiveLineDrawer* drawer)
{
	if (!lightData_.enabled) return;

	Vector3 dir = lightData_.direction;
	if (Length(dir) < 0.0001f) return;
	dir = Normalize(dir);

	const float distance = lightData_.distance;
	const float cosAngle = lightData_.cosAngle;
	if (distance <= 0.0f || cosAngle <= 0.0f) return;

	// 円錐の底面の半径
	const float sinAngle = std::sqrt(1.0f - cosAngle * cosAngle);
	const float radius = distance * sinAngle / cosAngle;
	const Vector3 baseCenter = lightData_.position + dir * distance;

	Vector4 color = lightData_.color;
	color.w = 1.0f;

	drawer->DrawLine(lightData_.position, baseCenter, color);
	drawer->DrawWireCircle(baseCenter, radius, dir, color, 24);
}
#endif // DEBUG
