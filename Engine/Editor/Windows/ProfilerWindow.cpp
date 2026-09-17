#include "ProfilerWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorHost.h"

#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Graphics/Resource/SrvManager.h"
#include "Graphics/Resource/TextureManager.h"
#include "World3D/Collider/CollisionManager.h"
#include "World3D/Light/LightManager.h"
#include "World3D/Object/Model/ModelManager.h"
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Object3dManager.h"
#include "Utility/DeltaTime.h"
#include "Utility/ScopeProfiler.h"

#include <imgui/imgui.h>

namespace {

void Row(const char* label, const char* format, ...)
{
	char buffer[128];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(label);
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(buffer);
}

/// <summary>
/// PROF_SCOPE / PROF_COUNT（Engine/Utility/ScopeProfiler.h）で集めた
/// フレーム内訳を出す。計測点を足したいときは測りたい場所にマクロを置くだけでよい。
/// </summary>
void DrawFrameTimeTable()
{
	const double frameMs = DeltaTime::GetUnscaledDeltaTime() * 1000.0;
	ImGui::Text("フレーム: %.2f ms (%.0f FPS)", frameMs, frameMs > 0.0 ? 1000.0 / frameMs : 0.0);

	if (!ImGui::CollapsingHeader("フレーム内訳", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	if (ImGui::Button("ピークをリセット")) {
		ScopeProfiler::ResetPeaks();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("PROF_SCOPE(\"名前\") / PROF_COUNT(\"名前\", 値) を置いた場所が並ぶ。\n"
			"平均は約60フレームの指数移動平均。Releaseでは計測ごと消える");
	}

	if (ImGui::BeginTable("##frame", 4,
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
		ImVec2(0.0f, 320.0f))) {
		ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("平均ms", ImGuiTableColumnFlags_WidthFixed, 60.0f);
		ImGui::TableSetupColumn("ピーク", ImGuiTableColumnFlags_WidthFixed, 60.0f);
		ImGui::TableSetupColumn("件数", ImGuiTableColumnFlags_WidthFixed, 70.0f);
		ImGui::TableHeadersRow();

		for (const ScopeProfiler::Entry& entry : ScopeProfiler::entries_) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(entry.name.c_str());
			ImGui::TableNextColumn();
			// フレームの1割を超えて食っている項目は目立たせる
			if (frameMs > 0.0 && entry.avgMs > frameMs * 0.1) {
				ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%.2f", entry.avgMs);
			} else {
				ImGui::Text("%.2f", entry.avgMs);
			}
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", entry.maxMs);
			ImGui::TableNextColumn();
			if (entry.counter != 0) {
				ImGui::Text("%lld", entry.counter);
			} else {
				ImGui::TextUnformatted("-");
			}
		}
		ImGui::EndTable();
	}
}

} // namespace

void Editor::DrawProfilerWindow()
{
	if (!EditorWindow::Begin("Profiler", EditorWindow::Category::kEngine, 0, false)) {
		return;
	}

	if (!HasContext()) {
		ImGui::TextDisabled("EditorContext がまだ設定されていません");
		EditorWindow::End();
		return;
	}

	DrawFrameTimeTable();

	if (!ImGui::CollapsingHeader("シーンの内訳", ImGuiTreeNodeFlags_DefaultOpen)) {
		EditorWindow::End();
		return;
	}

	if (!ImGui::BeginTable("##counts", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
		EditorWindow::End();
		return;
	}
	ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthFixed, 150.0f);
	ImGui::TableHeadersRow();

	if (Object3dManager* objects = Ctx().object3dManager) {
		const std::vector<Object3d*> all = objects->GetAllObject();
		size_t forward = 0;
		size_t renderers = 0;
		size_t hidden = 0;
		for (Object3d* object : all) {
			if (!object) {
				continue;
			}
			if (object->GetOption().drawPath == DrawPath::Forward) {
				++forward;
			}
			if (!object->GetIsDraw()) {
				++hidden;
			}
			renderers += object->GetRenderers().size();
		}
		Row("Object3d", "%zu", all.size());
		Row("  └ Forward / Deferred", "%zu / %zu", forward, all.size() - forward);
		Row("  └ 非表示", "%zu", hidden);
		Row("Renderer", "%zu", renderers);
	}

	if (CollisionManager* collision = Ctx().collisionManager) {
		Row("Collider", "%zu", collision->GetColliders().size());
	}

	if (LightManager* lights = Ctx().lightManager) {
		Row("Light", "%zu", lights->GetLights().size());
	}

	if (ModelManager* models = Ctx().modelManager) {
		Row("Model (通常 / スキン)", "%zu / %zu", models->models.size(), models->skinnedModels.size());
	}

	if (TextureManager* textures = Ctx().textureManager) {
		Row("Texture", "%zu", textures->GetLoadedTextureCount());
	}

	if (ParticleManager* particles = Ctx().particleManager) {
		Row("Particle Group / Emitter", "%zu / %zu",
			particles->GetParticleGroups().size(), particles->GetEmitters().size());
	}

	if (DirectXManager* dx = Ctx().dxManager) {
		if (SrvManager* srv = dx->GetSrvManager()) {
			const uint32_t used = srv->GetUsedCount();
			Row("SRV", "%u / %u", used, SrvManager::kMaxCount);
		}
	}

	Row("エディタウィンドウ", "%zu", EditorWindow::GetAllWindowNames().size());

	ImGui::EndTable();

	// SRVは足りなくなると LoadTexture の ASSERT_MSG で落ちるので、目に見えるところに出しておく
	if (DirectXManager* dx = Ctx().dxManager) {
		if (SrvManager* srv = dx->GetSrvManager()) {
			const float ratio = static_cast<float>(srv->GetUsedCount()) / static_cast<float>(SrvManager::kMaxCount);
			ImGui::Separator();
			ImGui::TextUnformatted("SRV 使用率");
			ImGui::ProgressBar(ratio, ImVec2(-FLT_MIN, 0.0f));
			if (ratio > 0.9f) {
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "残りわずかです");
			}
		}
	}

	EditorWindow::End();
}

#endif // _DEBUG
