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
