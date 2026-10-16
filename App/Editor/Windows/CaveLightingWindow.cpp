#include "AppEditorWindows.h"
#ifdef _DEBUG

#include "Editor/AppEditor.h"

#include <Debugger/GlobalVariables.h>
#include <Editor/Core/EditorHost.h>
#include <Graphics/Rendering/Environment/RenderEnvironment.h>
#include <Utility/GpuProfiler.h>

#include <imgui/imgui.h>

namespace {

// 洞窟の雰囲気の調整値（Resource/GlobalVariables/Environment/Environment_Cave.json）
constexpr const char* kGroup = "Environment_Cave";
// 漂う埃のパーティクルグループ（Resource/VFX/CaveDust.vfx.json）
constexpr const char* kDustGroup = "CaveDustMote";

GlobalVariables& Global() { return GlobalVariables::GetInstance(); }

// GlobalVariables の値を直接いじる。StageEnvironment が毎フレーム読み直すので、その場で画面に効く
void CheckboxParam(const char* label, const char* group, const char* key)
{
	if (Global().HasItem(group, key)) {
		ImGui::Checkbox(label, &Global().GetValueRef<bool>(group, key));
	}
}

void DragFloatParam(const char* label, const char* group, const char* key, float speed, float min, float max, const char* format = "%.3f")
{
	if (Global().HasItem(group, key)) {
		ImGui::DragFloat(label, &Global().GetValueRef<float>(group, key), speed, min, max, format);
	}
}

} // namespace

/// <summary>
/// 洞窟の埃・空間ライティングのデバッグ（埃パーティクル仕様書 10章）。
/// デバッグ表示の切り替え、主な機能の有効/無効、埃と光の筋の主な値、各パスの GPU 処理時間。
/// 細かい値は GlobalVariables（Environment_Cave）と VFX ウィンドウ（CaveDust）にある
/// </summary>
void AppEditor::DrawCaveLightingWindow()
{
	if (!EditorWindow::Begin("Cave Lighting", EditorWindow::Category::kGame)) {
		return;
	}

	// ── デバッグ表示 ──
	if (ImGui::CollapsingHeader("デバッグ表示", ImGuiTreeNodeFlags_DefaultOpen)) {
		RenderEnvironment& env = RenderEnvironment::GetInstance();
		int view = static_cast<int>(env.GetDebugView());
		bool changed = false;
		changed |= ImGui::RadioButton("通常", &view, 0);
		ImGui::SameLine();
		changed |= ImGui::RadioButton("光の筋だけ", &view, 1);
		changed |= ImGui::RadioButton("埃の受けた光", &view, 2);
		ImGui::SameLine();
		changed |= ImGui::RadioButton("埃の透明度", &view, 3);
		if (changed) {
			env.SetDebugView(static_cast<RenderEnvironment::DebugView>(view));
		}
		ImGui::TextDisabled("光源の範囲・スポットライトの円錐は Debug Draw メニューのライト");
	}

	if (!Global().HasItem(kGroup, "SkyVisible")) {
		ImGui::TextDisabled("洞窟のステージ（environment が Cave）に入ると調整できます");
	} else {
		// ── 機能ごとの有効・無効 ──
		if (ImGui::CollapsingHeader("有効・無効", ImGuiTreeNodeFlags_DefaultOpen)) {
			CheckboxParam("埃", kGroup, "DustEnabled");
			ImGui::SameLine();
			CheckboxParam("空間散乱（光の筋・松明の霞）", kGroup, "VolumetricEnabled");
			CheckboxParam("SSAO", kGroup, "SsaoEnabled");
			ImGui::SameLine();
			CheckboxParam("地面の霧", kGroup, "GroundMist_Enabled");
			ImGui::SameLine();
			CheckboxParam("水滴", kGroup, "DripEnabled");
		}

		// ── 埃 ──
		if (ImGui::CollapsingHeader("埃", ImGuiTreeNodeFlags_DefaultOpen)) {
			DragFloatParam("発生頻度（全体の倍率）", kGroup, "ParticleSpawnRate", 0.01f, 0.0f, 5.0f, "%.2f");
			DragFloatParam("プレイヤーのまわり[粒/秒]", kGroup, "DustPerSecond", 0.5f, 0.0f, 200.0f, "%.1f");
			DragFloatParam("天井の光の中[粒/秒]", kGroup, "DustConePerSecond", 0.5f, 0.0f, 200.0f, "%.1f");
			DragFloatParam("戦闘中の濃さ", kGroup, "DustBattleScale", 0.01f, 0.0f, 1.0f, "%.2f");
			DragFloatParam("透明度（最小）", kDustGroup, "BaseAlphaMin", 0.005f, 0.0f, 1.0f);
			DragFloatParam("透明度（最大）", kDustGroup, "BaseAlphaMax", 0.005f, 0.0f, 1.0f);
			DragFloatParam("暗所での見え方", kDustGroup, "AmbientVisibility", 0.01f, 0.0f, 1.0f, "%.2f");
			DragFloatParam("光への反応", kDustGroup, "LightResponse", 0.01f, 0.0f, 10.0f, "%.2f");
			DragFloatParam("光源の色の反映", kDustGroup, "ColorResponse", 0.01f, 0.0f, 1.0f, "%.2f");
			DragFloatParam("漂う強さ", kDustGroup, "NoiseMoveStrength", 0.001f, 0.0f, 1.0f);
			DragFloatParam("近距離フェード（終わり）", kDustGroup, "NearFadeEnd", 0.05f, 0.0f, 10.0f, "%.2f");
			DragFloatParam("遠距離フェード（終わり）", kDustGroup, "FarFadeEnd", 0.1f, 0.0f, 100.0f, "%.1f");
			DragFloatParam("壁との交差のフェード", kDustGroup, "SoftDistance", 0.01f, 0.0f, 5.0f, "%.2f");
		}

		// ── 光の筋 ──
		if (ImGui::CollapsingHeader("光の筋（天井のスポットライト）", ImGuiTreeNodeFlags_DefaultOpen)) {
			DragFloatParam("強さ", kGroup, "SpotVolumetricStrength", 0.001f, 0.0f, 1.0f);
			if (Global().HasItem(kGroup, "SpotVolumetricSteps")) {
				ImGui::SliderInt("サンプル数", &Global().GetValueRef<int32_t>(kGroup, "SpotVolumetricSteps"), 1, 48);
			}
			DragFloatParam("霧の濃淡の細かさ", kGroup, "SpotFogNoiseScale", 0.005f, 0.0f, 4.0f);
			DragFloatParam("霧の濃淡の強さ", kGroup, "SpotFogNoiseInfluence", 0.01f, 0.0f, 1.0f, "%.2f");
			DragFloatParam("前方散乱", kGroup, "SpotPhaseG", 0.01f, -0.9f, 0.9f, "%.2f");
			DragFloatParam("光源の近くを薄く[m]", kGroup, "SpotApexFade", 0.1f, 0.0f, 30.0f, "%.1f");
			DragFloatParam("戦闘中の濃さ", kGroup, "SpotVolumetricBattleScale", 0.01f, 0.0f, 1.0f, "%.2f");
		}
	}

	// ── GPU 処理時間 ──
	if (ImGui::CollapsingHeader("GPU 処理時間", ImGuiTreeNodeFlags_DefaultOpen)) {
		const GpuProfiler& gpu = GpuProfiler::GetInstance();
		ImGui::Text("合計 %.2f ms", gpu.TotalMs());
		if (ImGui::BeginTable("##caveGpu", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("パス", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("平均ms", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableSetupColumn("ピーク", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableHeadersRow();
			for (const GpuProfiler::Entry& entry : gpu.Entries()) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(entry.name.c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", entry.avgMs);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", entry.maxMs);
			}
			ImGui::EndTable();
		}
		ImGui::TextDisabled("SSAO は Pass:SSAO、光の筋・松明の霞は Pass:Lighting に含まれる");
	}

	EditorWindow::End();
}

#endif // _DEBUG
