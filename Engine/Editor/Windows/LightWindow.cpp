#include "LightWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorHost.h"
#include "Editor/Core/EditorMenuBar.h"

#include "World3D/Light/DirectionalLight.h"
#include "World3D/Light/DynamicPointLight.h"
#include "World3D/Light/LightManager.h"
#include "World3D/Light/PointLight.h"
#include "World3D/Light/SpotLight.h"

#include <imgui/imgui.h>
#include <string>

namespace {

// 選択はインデックスではなく名前で持つ。削除するとインデックスがずれるため
std::string g_selectedLight;

char g_newLightName[64] = "NewLight";
int g_newLightType = 1; // 既定はPoint

const char* const kTypeLabels[] = { "Directional", "Point", "Spot" };

const char* TypeLabel(uint32_t type)
{
	return (type < IM_ARRAYSIZE(kTypeLabels)) ? kTypeLabels[type] : "?";
}

std::string MakeUniqueLightName(LightManager& manager, const std::string& desired)
{
	const std::string base = desired.empty() ? std::string("Light") : desired;
	auto exists = [&manager](const std::string& name) {
		for (const auto& light : manager.GetLights()) {
			if (light && light->GetName() == name) {
				return true;
			}
		}
		return false;
		};

	if (!exists(base)) {
		return base;
	}
	for (int i = 1; i < 10000; ++i) {
		std::string candidate = base + "_" + std::to_string(i);
		if (!exists(candidate)) {
			return candidate;
		}
	}
	return base;
}

void DrawAddPopup(LightManager& manager)
{
	if (!ImGui::BeginPopup("##AddLight")) {
		return;
	}

	ImGui::TextDisabled("ライトを追加する");
	ImGui::Separator();
	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputText("名前", g_newLightName, sizeof(g_newLightName));
	ImGui::SetNextItemWidth(200.0f);
	ImGui::Combo("種類", &g_newLightType, kTypeLabels, IM_ARRAYSIZE(kTypeLabels));

	// Directional/Point/Spot はコンストラクタの中で GlobalVariables に項目を登録し、
	// Resource/GlobalVariables/Light/<名前>.json から前回値を読む。
	// つまり名前がそのまま保存先になる
	ImGui::TextDisabled("パラメータは Light/<名前>.json に保存されます");

	ImGui::Separator();
	if (ImGui::Button("追加", ImVec2(100.0f, 0.0f))) {
		const std::string name = MakeUniqueLightName(manager, g_newLightName);
		std::unique_ptr<BaseLight> light;
		switch (g_newLightType) {
		case 0: light = std::make_unique<DirectionalLight>(name); break;
		case 1: light = std::make_unique<PointLight>(name); break;
		case 2: light = std::make_unique<SpotLight>(name); break;
		default: break;
		}
		if (light) {
			manager.AddLight(std::move(light));
			g_selectedLight = name;
			EditorMenuBar::ShowToast("%s を追加しました", name.c_str());
		}
		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("閉じる", ImVec2(100.0f, 0.0f))) {
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}

} // namespace

void Editor::DrawLightWindow()
{
	if (!EditorWindow::Begin("Light Manager", EditorWindow::Category::kLighting)) {
		return;
	}

	LightManager* manager = Ctx().lightManager;
	if (!manager) {
		ImGui::TextDisabled("EditorContext がまだ設定されていません");
		EditorWindow::End();
		return;
	}

	if (ImGui::Button("+ 追加")) {
		ImGui::OpenPopup("##AddLight");
	}
	DrawAddPopup(*manager);

	ImGui::SameLine();
	ImGui::TextDisabled("%zu 個", manager->GetLights().size());
	ImGui::Separator();

	const auto& lights = manager->GetLights();
	if (lights.empty()) {
		ImGui::TextDisabled("(ライトがありません)");
		EditorWindow::End();
		return;
	}

	// --- 一覧 ---
	BaseLight* selected = nullptr;
	if (ImGui::BeginChild("##lightList", ImVec2(0.0f, 130.0f), ImGuiChildFlags_Borders)) {
		for (const auto& light : lights) {
			if (!light) {
				continue;
			}
			const std::string& name = light->GetName();
			const LightData& data = light->GetLightData();

			ImGui::PushID(light.get());
			if (ImGui::Selectable(name.c_str(), g_selectedLight == name)) {
				g_selectedLight = name;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("  [%s]%s", TypeLabel(data.type), data.enabled ? "" : "  (無効)");
			ImGui::PopID();
		}
	}
	ImGui::EndChild();

	for (const auto& light : lights) {
		if (light && light->GetName() == g_selectedLight) {
			selected = light.get();
			break;
		}
	}

	ImGui::Separator();

	if (!selected) {
		ImGui::TextDisabled("ライトを選んでください");
		EditorWindow::End();
		return;
	}

	// --- 選択中ライトの中身。UIは各ライトが持っているものをそのまま使う ---
	ImGui::TextUnformatted(selected->GetName().c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("[%s]", TypeLabel(selected->GetLightData().type));

	selected->DrawLightEditor();

	ImGui::Separator();
	if (ImGui::Button("このライトを削除")) {
		EditorMenuBar::ShowToast("%s を削除しました", selected->GetName().c_str());
		// RemoveLight は unique_ptr ごと消すので、これ以降 selected を触らないこと
		manager->RemoveLight(selected);
		g_selectedLight.clear();
	}

	EditorWindow::End();
}

#endif // _DEBUG
