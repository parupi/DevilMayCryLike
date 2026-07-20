#include "DynamicPointLight.h"
#ifdef _DEBUG
#include <imgui.h>
#include "World3D/Primitive/PrimitiveLineDrawer.h"
#endif // DEBUG

DynamicPointLight::DynamicPointLight(const std::string& name)
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

void DynamicPointLight::Initialize()
{
}

void DynamicPointLight::Update()
{
	// 所有者（キャラクター側）が Setter で毎フレーム更新するため何もしない
}

#ifdef _DEBUG
void DynamicPointLight::DrawLightEditor()
{
	ImGui::TextUnformatted("Runtime-driven light (owner overwrites values every frame)");

	bool enabled = lightData_.enabled != 0;
	if (ImGui::Checkbox("Enabled", &enabled)) {
		lightData_.enabled = enabled ? 1 : 0;
	}
	ImGui::ColorEdit4("Color", &lightData_.color.x);
	ImGui::DragFloat3("Position", &lightData_.position.x, 0.1f);
	ImGui::DragFloat("Intensity", &lightData_.intensity, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("Radius", &lightData_.radius, 0.1f, 0.0f, 1000.0f);
	ImGui::DragFloat("Decay", &lightData_.decay, 0.01f, 0.0f, 10.0f);
}

void DynamicPointLight::DrawDebug(PrimitiveLineDrawer* drawer)
{
	drawer->DrawWireSphere(lightData_.position, lightData_.radius, lightData_.color);
}
#endif // DEBUG
