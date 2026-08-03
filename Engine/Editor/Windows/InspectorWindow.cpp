#include "InspectorWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorAssetUtil.h"
#include "Editor/Core/EditorHost.h"
#include "Editor/Core/EditorMenuBar.h"
#include "Editor/Core/EditorSelection.h"

#include "Math/MathUtils.h"
#include "Math/Quaternion.h"
#include "World3D/Collider/BaseCollider.h"
#include "World3D/Object/Model/BaseModel.h"
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/Object/Renderer/BaseRenderer.h"
#include "World3D/WorldTransform.h"

#include <imgui/imgui.h>

namespace {

const char* const kDrawPathLabels[] = { "Forward", "Deferred" };
const char* const kBlendModeLabels[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
const char* const kShapeLabels[] = { "AABB", "Sphere", "OBB" };

void DrawTransformSection(WorldTransform* transform)
{
	if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	ImGui::DragFloat3("位置", &transform->GetTranslation().x, 0.05f);
	ImGui::DragFloat3("スケール", &transform->GetScale().x, 0.01f);

	// 回転はクォータニオンで持っているので、オイラー角に直して往復させると誤差が溜まる。
	// そこで「加算」と「絶対指定」を分け、加算のほうは毎フレーム 0 に戻る差分として扱う
	Vector3 delta{ 0.0f, 0.0f, 0.0f };
	if (ImGui::DragFloat3("回転を加算 (deg)", &delta.x, 0.1f)) {
		transform->GetRotation() = transform->GetRotation() * Normalize(EulerDegree(delta));
	}

	static Vector3 absoluteEuler{ 0.0f, 0.0f, 0.0f };
	ImGui::DragFloat3("##absEuler", &absoluteEuler.x, 0.1f);
	ImGui::SameLine();
	if (ImGui::Button("絶対指定")) {
		transform->GetRotation() = Normalize(EulerDegree(absoluteEuler));
	}

	const Quaternion& rotation = transform->GetRotation();
	ImGui::TextDisabled("quaternion  x%.3f y%.3f z%.3f w%.3f",
		rotation.x, rotation.y, rotation.z, rotation.w);

	if (ImGui::Button("回転リセット")) {
		transform->GetRotation() = Identity();
	}
	ImGui::SameLine();
	if (ImGui::Button("スケールリセット")) {
		transform->GetScale() = Vector3(1.0f, 1.0f, 1.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("位置リセット")) {
		transform->GetTranslation() = Vector3(0.0f, 0.0f, 0.0f);
	}

	const Vector3 worldPos = transform->GetWorldPos();
	ImGui::TextDisabled("world  %.2f, %.2f, %.2f", worldPos.x, worldPos.y, worldPos.z);
	if (transform->GetParent()) {
		ImGui::SameLine();
		ImGui::TextDisabled("(親あり)");
	}
}

void DrawDrawOptionSection(Object3d* object)
{
	if (!ImGui::CollapsingHeader("描画設定", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	bool isDraw = object->GetIsDraw();
	if (ImGui::Checkbox("表示する", &isDraw)) {
		object->SetIsDraw(isDraw);
	}

	// DrawOption は Object3d の非公開の入れ子型なので、型名を書かずに auto& で受ける
	auto& option = object->GetOption();

	int drawPath = static_cast<int>(option.drawPath);
	if (ImGui::Combo("描画パス", &drawPath, kDrawPathLabels, IM_ARRAYSIZE(kDrawPathLabels))) {
		option.drawPath = static_cast<DrawPath>(drawPath);
	}

	int blendMode = static_cast<int>(option.blendMode);
	if (ImGui::Combo("ブレンド", &blendMode, kBlendModeLabels, IM_ARRAYSIZE(kBlendModeLabels))) {
		option.blendMode = static_cast<BlendMode>(blendMode);
	}
	if (option.drawPath == DrawPath::Deferred) {
		ImGui::TextDisabled("※ ブレンドは Forward のときだけ効きます");
	}
}

void DrawRendererSection(Object3d* object)
{
	const auto& renderers = object->GetRenderers();

	char header[64];
	snprintf(header, sizeof(header), "Renderer (%zu)###renderers", renderers.size());
	if (!ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	if (renderers.empty()) {
		ImGui::TextDisabled("(レンダラーがありません)");
		return;
	}

	for (size_t i = 0; i < renderers.size(); ++i) {
		BaseRenderer* renderer = renderers[i];
		if (!renderer) {
			continue;
		}
		ImGui::PushID(static_cast<int>(i));

		ImGui::Text("%zu: %s", i, renderer->name_.c_str());
		const std::string modelName = Editor::FindLoadedModelName(renderer->GetModel());
		if (!modelName.empty()) {
			ImGui::SameLine();
			ImGui::TextDisabled("[%s]", modelName.c_str());
		}

		// レンダラー個別のUIは既存の DebugGui をそのまま使う
		// （中身は TreeNode なので、ここでウィンドウが増えることはない）
		renderer->DebugGui(i);

		ImGui::PopID();
	}
}

void DrawColliderSection(Object3d* object)
{
	const auto& colliders = object->GetColliders();

	char header[64];
	snprintf(header, sizeof(header), "Collider (%zu)###colliders", colliders.size());
	if (!ImGui::CollapsingHeader(header)) {
		return;
	}

	if (colliders.empty()) {
		ImGui::TextDisabled("(コライダーがありません)");
		return;
	}

	for (BaseCollider* collider : colliders) {
		if (!collider) {
			continue;
		}
		const int shape = static_cast<int>(collider->GetShapeType());
		const char* shapeLabel = (shape >= 0 && shape < IM_ARRAYSIZE(kShapeLabels)) ? kShapeLabels[shape] : "?";
		ImGui::BulletText("%s  [%s]%s", collider->name_.c_str(), shapeLabel,
			collider->isAlive ? "" : "  (削除済み)");
	}
}

} // namespace

void Editor::DrawInspectorWindow()
{
	if (!EditorWindow::Begin("Inspector", EditorWindow::Category::kWorld)) {
		return;
	}

	Object3d* object = GetSelectedObject();
	if (!object) {
		const std::string& name = GetSelectedObjectName();
		if (name.empty()) {
			ImGui::TextDisabled("Hierarchy でオブジェクトを選んでください");
		} else {
			// 選択中に削除されたケース。名前は残しつつ、消えたことが分かるようにする
			ImGui::TextDisabled("\"%s\" は見つかりません（削除された？）", name.c_str());
			if (ImGui::Button("選択を解除")) {
				ClearObjectSelection();
			}
		}
		EditorWindow::End();
		return;
	}

	ImGui::TextUnformatted(object->name_.c_str());
	ImGui::SameLine();
	ImGui::TextDisabled(object->isAlive ? "" : "(削除待ち)");
	ImGui::Separator();

	DrawTransformSection(object->GetWorldTransform());
	DrawDrawOptionSection(object);
	DrawRendererSection(object);
	DrawColliderSection(object);

	ImGui::Separator();
	if (ImGui::Button("このオブジェクトを削除")) {
		if (Object3dManager* manager = Ctx().object3dManager) {
			manager->DeleteObject(object->name_);
			EditorMenuBar::ShowToast("%s を削除しました", object->name_.c_str());
			ClearObjectSelection();
		}
	}

	EditorWindow::End();
}

#endif // _DEBUG
