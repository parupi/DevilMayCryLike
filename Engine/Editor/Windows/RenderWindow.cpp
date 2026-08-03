#include "RenderWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorHost.h"

#include "Graphics/Rendering/PostEffect/BaseOffScreen.h"
#include "Graphics/Rendering/PostEffect/BloomEffect.h"
#include "Graphics/Rendering/PostEffect/GaussianEffect.h"
#include "Graphics/Rendering/PostEffect/GrayEffect.h"
#include "Graphics/Rendering/PostEffect/OffScreenManager.h"
#include "Graphics/Rendering/PostEffect/SmoothEffect.h"
#include "Graphics/Rendering/PostEffect/VignetteEffect.h"
#include "Graphics/Rendering/Shadow/CascadedShadowMap.h"
#include "Graphics/Rendering/Sky/SkySystem.h"
#include "World3D/Camera/BaseCamera.h"
#include "World3D/Light/LightManager.h"

#include <imgui/imgui.h>
#include <string>

namespace {

// チェーンの並び替えは描画中に vector を触ることになるので、
// 「このフレームで何をするか」だけ覚えて、一覧を描き終えてから適用する
int g_pendingMoveIndex = -1;
int g_pendingMoveDirection = 0;

// パラメータを出す対象。名前で持つ（並び替えでインデックスがずれるため）
std::string g_selectedEffect;

// --- エフェクト個別のパラメータ ---
// もとは各エフェクトの Update() の中で ImGui を呼んでいた。
// 型ごとに持つものが違うので、ここで dynamic_cast して振り分ける
void DrawEffectParams(BaseOffScreen* effect)
{
	if (auto* bloom = dynamic_cast<BloomEffect*>(effect)) {
		BloomEffect::BloomSettings& s = bloom->GetSettings();
		ImGui::DragFloat("threshold", &s.threshold, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("softKnee", &s.softKnee, 0.01f, 0.01f, 1.0f);
		ImGui::DragFloat("blurRadius", &s.blurRadius, 0.05f, 0.0f, 8.0f);
		ImGui::DragFloat("intensity", &s.intensity, 0.01f, 0.0f, 4.0f);
		return;
	}
	// HitVignetteEffect など VignetteEffect を継承したものもここで拾える
	if (auto* vignette = dynamic_cast<VignetteEffect*>(effect)) {
		VignetteEffect::VignetteEffectData& d = vignette->GetEffectData();
		ImGui::DragFloat("radius", &d.radius, 0.01f);
		ImGui::DragFloat("intensity", &d.intensity, 0.01f);
		ImGui::DragFloat("softness", &d.softness, 0.01f);
		float color[3] = { d.colorR, d.colorG, d.colorB };
		if (ImGui::ColorEdit3("edgeColor", color)) {
			vignette->SetColor(color[0], color[1], color[2]);
		}
		return;
	}
	if (auto* gauss = dynamic_cast<GaussianEffect*>(effect)) {
		if (auto* d = gauss->GetEffectData()) {
			ImGui::SliderFloat("Sigma", &d->sigma, 0.1f, 10.0f, "%.2f");
			ImGui::SliderFloat("Blur Strength", &d->blurStrength, 0.0f, 5.0f, "%.2f");
			ImGui::Combo("Alpha Mode", reinterpret_cast<int*>(&d->alphaMode), "Fixed 1.0\0Sample Alpha\0\0");
			ImGui::DragFloat2("UV Clamp Min", &d->uvClampMin.x, 0.01f, -1.0f, 1.0f, "%.2f");
			ImGui::DragFloat2("UV Clamp Max", &d->uvClampMax.x, 0.01f, -1.0f, 1.0f, "%.2f");
		}
		return;
	}
	if (auto* gray = dynamic_cast<GrayEffect*>(effect)) {
		if (auto* d = gray->GetEffectData()) {
			ImGui::DragFloat("intensity", &d->intensity, 0.01f);
		}
		return;
	}
	if (auto* smooth = dynamic_cast<SmoothEffect*>(effect)) {
		if (auto* d = smooth->GetEffectData()) {
			ImGui::DragFloat("blurStrength", &d->blurStrength, 0.01f);
			ImGui::DragInt("iterations", &d->iterations);
		}
		return;
	}

	ImGui::TextDisabled("調整できるパラメータはありません");
}

void DrawPostEffectChain(OffScreenManager& manager)
{
	const std::vector<BaseOffScreen*> effects = manager.GetEffects();

	ImGui::TextDisabled("上にあるものから順に適用されます（%zu 個）", effects.size());

	if (effects.empty()) {
		ImGui::TextDisabled("(ポストエフェクトが登録されていません)");
		return;
	}

	if (ImGui::BeginTable("##effects", 3,
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
		ImGui::TableSetupColumn("##order", ImGuiTableColumnFlags_WidthFixed, 70.0f);
		ImGui::TableSetupColumn("有効", ImGuiTableColumnFlags_WidthFixed, 40.0f);
		ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		for (size_t i = 0; i < effects.size(); ++i) {
			BaseOffScreen* effect = effects[i];
			if (!effect) {
				continue;
			}
			ImGui::PushID(static_cast<int>(i));
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::BeginDisabled(i == 0);
			if (ImGui::SmallButton("▲")) {
				g_pendingMoveIndex = static_cast<int>(i);
				g_pendingMoveDirection = -1;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(i + 1 >= effects.size());
			if (ImGui::SmallButton("▼")) {
				g_pendingMoveIndex = static_cast<int>(i);
				g_pendingMoveDirection = 1;
			}
			ImGui::EndDisabled();

			ImGui::TableNextColumn();
			bool active = effect->IsActive();
			if (ImGui::Checkbox("##active", &active)) {
				effect->SetActive(active);
			}

			ImGui::TableNextColumn();
			const std::string name = effect->GetName();
			if (!active) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			}
			if (ImGui::Selectable(name.c_str(), g_selectedEffect == name)) {
				g_selectedEffect = name;
			}
			if (!active) {
				ImGui::PopStyleColor();
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	if (g_pendingMoveIndex >= 0) {
		manager.MoveEffect(static_cast<size_t>(g_pendingMoveIndex), g_pendingMoveDirection);
		g_pendingMoveIndex = -1;
		g_pendingMoveDirection = 0;
	}

	// --- 選択したエフェクトのパラメータ ---
	ImGui::Separator();
	BaseOffScreen* selected = g_selectedEffect.empty() ? nullptr : manager.FindEffect(g_selectedEffect);
	if (!selected) {
		ImGui::TextDisabled("エフェクト名をクリックすると設定が出ます");
		return;
	}
	ImGui::TextUnformatted(selected->GetName().c_str());
	DrawEffectParams(selected);
}

void DrawShadowSection(CascadedShadowMap& csm)
{
	ImGui::DragFloat("Light Distance", &csm.GetShadowDistance(), 1.0f, 10.0f, 500.0f, "%.1f");
	ImGui::DragFloat("Shadow Far", &csm.GetShadowFar(), 1.0f, 1.0f, 2000.0f, "%.1f");
	ImGui::SliderFloat("Split Lambda", &csm.GetSplitLambda(), 0.0f, 1.0f, "%.2f");
	ImGui::TextDisabled("シャドウマップ %u px", csm.GetShadowMapSize());

	ImGui::Spacing();
	ImGui::TextUnformatted("カスケード分割深度 (view-space Z)");
	const CascadeData* cascades = csm.GetCascadeData();
	for (uint32_t i = 0; i < kCascadeCount; ++i) {
		ImGui::BulletText("Cascade %u far: %.1f", i, cascades[i].splitDepth);
	}

	const std::vector<LightData>& lights = csm.GetLights();
	if (!lights.empty()) {
		ImGui::TextDisabled("ライト方向  (%.2f, %.2f, %.2f)",
			lights[0].direction.x, lights[0].direction.y, lights[0].direction.z);
	}
	if (const BaseCamera* camera = csm.GetCamera()) {
		const Vector3 position = const_cast<BaseCamera*>(camera)->GetTranslate();
		ImGui::TextDisabled("カメラ位置  (%.1f, %.1f, %.1f)", position.x, position.y, position.z);
	}
}

void DrawSkySection(SkySystem& sky)
{
	const int32_t index = sky.GetEnvironmentMapIndex();
	if (index < 0) {
		ImGui::TextDisabled("スカイボックス未設定");
	} else {
		ImGui::Text("環境マップ SRV: %d", index);
	}
}

} // namespace

void Editor::DrawRenderWindow()
{
	if (!EditorWindow::Begin("Render", EditorWindow::Category::kPostEffect)) {
		return;
	}

	if (OffScreenManager* offscreen = Ctx().offScreenManager) {
		if (ImGui::CollapsingHeader("ポストエフェクトのチェーン", ImGuiTreeNodeFlags_DefaultOpen)) {
			DrawPostEffectChain(*offscreen);
		}
	} else {
		ImGui::TextDisabled("EditorContext がまだ設定されていません");
	}

	// もとは CascadedShadowMap::DrawDebugUI() が "Shadow Map (CSM)" ウィンドウを開いていた
	if (LightManager* lights = Ctx().lightManager) {
		if (CascadedShadowMap* csm = lights->GetCSM()) {
			if (ImGui::CollapsingHeader("シャドウ (CSM)")) {
				DrawShadowSection(*csm);
			}
		}
	}

	if (SkySystem* sky = Ctx().skySystem) {
		if (ImGui::CollapsingHeader("スカイボックス")) {
			DrawSkySection(*sky);
		}
	}

	EditorWindow::End();
}

#endif // _DEBUG
