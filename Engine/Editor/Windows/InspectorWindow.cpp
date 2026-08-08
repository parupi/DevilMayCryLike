#include "InspectorWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorAssetUtil.h"
#include "Editor/Core/EditorHost.h"
#include "Editor/Core/EditorMenuBar.h"
#include "Editor/Core/EditorSelection.h"
#include "Editor/Core/EditorUndo.h"

#include "Math/MathUtils.h"
#include "Math/Quaternion.h"
#include "World3D/Collider/AABBCollider.h"
#include "World3D/Collider/BaseCollider.h"
#include "World3D/Collider/CollisionManager.h"
#include "World3D/Collider/OBBCollider.h"
#include "World3D/Collider/SphereCollider.h"
#include "World3D/Object/Model/BaseModel.h"
#include "World3D/Object/Model/ModelManager.h"
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/Object/Renderer/BaseRenderer.h"
#include "World3D/Object/Renderer/ModelRenderer.h"
#include "World3D/WorldTransform.h"

#include <algorithm>
#include <imgui/imgui.h>
#include <vector>

namespace {

const char* const kDrawPathLabels[] = { "Forward", "Deferred" };
const char* const kBlendModeLabels[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
const char* const kShapeLabels[] = { "AABB", "Sphere", "OBB" };

// ドラッグ系ウィジェットの直後に呼ぶ。掴んだ瞬間に変更前の値を控え、離した瞬間に履歴へ積む
void TrackTransformEdit(Object3d* object)
{
	if (ImGui::IsItemActivated()) {
		EditorUndo::BeginTransformEdit(object);
	}
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		EditorUndo::EndTransformEdit(object);
	}
}

void DrawTransformSection(Object3d* object)
{
	if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}
	WorldTransform* transform = object->GetWorldTransform();

	ImGui::DragFloat3("位置", &transform->GetTranslation().x, 0.05f);
	TrackTransformEdit(object);
	ImGui::DragFloat3("スケール", &transform->GetScale().x, 0.01f);
	TrackTransformEdit(object);

	// 回転はクォータニオンで持っているので、オイラー角に直して往復させると誤差が溜まる。
	// そこで「加算」と「絶対指定」を分け、加算のほうは毎フレーム 0 に戻る差分として扱う
	Vector3 delta{ 0.0f, 0.0f, 0.0f };
	if (ImGui::DragFloat3("回転を加算 (deg)", &delta.x, 0.1f)) {
		transform->GetRotation() = transform->GetRotation() * Normalize(EulerDegree(delta));
	}
	TrackTransformEdit(object);

	static Vector3 absoluteEuler{ 0.0f, 0.0f, 0.0f };
	ImGui::DragFloat3("##absEuler", &absoluteEuler.x, 0.1f);
	ImGui::SameLine();
	if (ImGui::Button("絶対指定")) {
		EditorUndo::BeginTransformEdit(object);
		transform->GetRotation() = Normalize(EulerDegree(absoluteEuler));
		EditorUndo::EndTransformEdit(object);
	}

	const Quaternion& rotation = transform->GetRotation();
	ImGui::TextDisabled("quaternion  x%.3f y%.3f z%.3f w%.3f",
		rotation.x, rotation.y, rotation.z, rotation.w);

	if (ImGui::Button("回転リセット")) {
		EditorUndo::BeginTransformEdit(object);
		transform->GetRotation() = Identity();
		EditorUndo::EndTransformEdit(object);
	}
	ImGui::SameLine();
	if (ImGui::Button("スケールリセット")) {
		EditorUndo::BeginTransformEdit(object);
		transform->GetScale() = Vector3(1.0f, 1.0f, 1.0f);
		EditorUndo::EndTransformEdit(object);
	}
	ImGui::SameLine();
	if (ImGui::Button("位置リセット")) {
		EditorUndo::BeginTransformEdit(object);
		transform->GetTranslation() = Vector3(0.0f, 0.0f, 0.0f);
		EditorUndo::EndTransformEdit(object);
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

// 使用するモデルの差し替え。Ground / Prop と、エディタでモデルを貼った Object3d が対象。
// ModelRenderer だけ差し替えるとステージデータに残らないので、Object3d 側の名前も一緒に更新する
void DrawModelSection(Object3d* object)
{
	// モデル名を持たないもの（プリミティブ・イベント・空）には出さない
	if (object->GetModelName().empty()) {
		return;
	}
	if (!ImGui::CollapsingHeader("モデル", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	const std::vector<std::string>& modelFolders = Editor::CachedModelFolders();
	if (modelFolders.empty()) {
		ImGui::TextDisabled("Resource/Models にモデルがありません");
		return;
	}

	ImGui::SetNextItemWidth(-90.0f);
	if (ImGui::BeginCombo("使うモデル", object->GetModelName().c_str())) {
		for (const std::string& name : modelFolders) {
			const bool selected = (name == object->GetModelName());
			if (ImGui::Selectable(name.c_str(), selected) && !selected) {
				// FindModel は読み込み済みしか返さないので、先に読む
				if (ModelManager* models = Editor::Ctx().modelManager) {
					models->LoadModel(name);
				}
				object->SetModelName(name);
				for (BaseRenderer* renderer : object->GetRenderers()) {
					if (auto* modelRenderer = dynamic_cast<ModelRenderer*>(renderer)) {
						modelRenderer->SetModel(name);
					}
				}
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::SmallButton("再スキャン")) {
		Editor::CachedModelFolders(true);
	}
}

// コライダーを1つ追加する。所有は CollisionManager、Object3d は生ポインタで参照する
void AddCollider(Object3d* object, std::unique_ptr<BaseCollider> collider)
{
	BaseCollider* raw = collider.get();
	CollisionManager::GetInstance().AddCollider(std::move(collider));
	object->AddCollider(raw);
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
	}

	// 削除は列挙の後にまとめて行う（回している最中に配列を触らない）
	BaseCollider* pendingRemove = nullptr;

	for (size_t i = 0; i < colliders.size(); ++i) {
		BaseCollider* collider = colliders[i];
		if (!collider) {
			continue;
		}
		ImGui::PushID(static_cast<int>(i));

		const int shape = static_cast<int>(collider->GetShapeType());
		const char* shapeLabel = (shape >= 0 && shape < IM_ARRAYSIZE(kShapeLabels)) ? kShapeLabels[shape] : "?";
		ImGui::SeparatorText(shapeLabel);

		if (auto* obb = dynamic_cast<OBBCollider*>(collider)) {
			OBBData& data = obb->GetColliderData();
			ImGui::DragFloat3("オフセット", &data.offset.x, 0.05f);
			// 入力はフルサイズのほうが直感的なので、半分にして持つ
			Vector3 size = data.halfExtents * 2.0f;
			if (ImGui::DragFloat3("サイズ", &size.x, 0.05f, 0.0f, 10000.0f)) {
				data.halfExtents = size * 0.5f;
			}
			ImGui::Checkbox("有効", &data.isActive);

		} else if (auto* sphere = dynamic_cast<SphereCollider*>(collider)) {
			SphereData& data = sphere->GetColliderData();
			ImGui::DragFloat3("オフセット", &data.offset.x, 0.05f);
			ImGui::DragFloat("半径", &data.radius, 0.05f, 0.0f, 10000.0f);
			ImGui::Checkbox("有効", &data.isActive);

		} else if (auto* aabb = dynamic_cast<AABBCollider*>(collider)) {
			AABBData& data = aabb->GetColliderData();
			ImGui::DragFloat3("最小", &data.offsetMin.x, 0.05f);
			ImGui::DragFloat3("最大", &data.offsetMax.x, 0.05f);
			ImGui::Checkbox("有効", &data.isActive);
			ImGui::TextDisabled("※ ステージデータは AABB を保存しません");

		} else {
			ImGui::TextDisabled("このコライダーは編集できません");
		}

		if (ImGui::SmallButton("このコライダーを削除")) {
			pendingRemove = collider;
		}

		ImGui::PopID();
	}

	if (pendingRemove) {
		object->RemoveCollider(pendingRemove);
	}

	ImGui::Separator();
	// ステージデータが持てるのは1オブジェクト1コライダーなので、既にあるなら足させない
	ImGui::BeginDisabled(!object->GetColliders().empty());
	if (ImGui::Button("OBBを追加")) {
		auto collider = std::make_unique<OBBCollider>(object->name_);
		collider->GetColliderData().halfExtents = { 1.0f, 1.0f, 1.0f };
		AddCollider(object, std::move(collider));
	}
	ImGui::SameLine();
	if (ImGui::Button("球を追加")) {
		auto collider = std::make_unique<SphereCollider>(object->name_);
		collider->GetColliderData().radius = 1.0f;
		AddCollider(object, std::move(collider));
	}
	ImGui::EndDisabled();
	if (!object->GetColliders().empty()) {
		ImGui::TextDisabled("※ 保存されるのは先頭の1つだけです");
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
	ImGui::TextDisabled("[%s]%s", object->GetClassName().c_str(), object->isAlive ? "" : " (削除待ち)");
	if (!object->IsStageObject()) {
		ImGui::TextDisabled("このオブジェクトはステージデータに保存されません");
	}
	ImGui::Separator();

	DrawTransformSection(object);
	DrawDrawOptionSection(object);
	DrawModelSection(object);
	DrawRendererSection(object);
	DrawColliderSection(object);

	// App が AddInspectorSection で足した項目（ライトやイベントの設定など）
	DrawInspectorSections(object);

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
