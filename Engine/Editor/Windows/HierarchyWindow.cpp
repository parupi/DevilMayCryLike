#include "HierarchyWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorAssetUtil.h"
#include "Editor/Core/EditorHost.h"
#include "Editor/Core/EditorMenuBar.h"
#include "Editor/Core/EditorSelection.h"

#include "World3D/Object/Model/ModelManager.h"
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/Object/Renderer/ModelRenderer.h"
#include "World3D/Object/Renderer/PrimitiveRenderer.h"
#include "World3D/Object/Renderer/RendererManager.h"
#include "World3D/WorldTransform.h"

#include <algorithm>
#include <cctype>
#include <imgui/imgui.h>

namespace {

// --- 生成ダイアログの状態 ---

enum class CreateKind {
	Model,      // Resource/Models のモデルを貼る
	Primitive,  // Plane / Ring / Cylinder
	Empty,      // レンダラー無し（後から付ける・親にするなど）
};

char g_newName[64] = "NewObject";
CreateKind g_createKind = CreateKind::Model;
int g_selectedModelIndex = 0;
int g_primitiveIndex = 0;
char g_primitiveTexture[64] = "white.png";
float g_newPosition[3] = { 0.0f, 0.0f, 0.0f };

// Resource/Models の走査結果。毎フレーム掘るのは重いので保持しておく
std::vector<std::string> g_modelFolders;
bool g_modelFoldersScanned = false;

char g_filter[64] = "";

const char* const kPrimitiveLabels[] = { "Plane", "Ring", "Cylinder" };
constexpr PrimitiveType kPrimitiveTypes[] = {
	PrimitiveType::Plane, PrimitiveType::Ring, PrimitiveType::Cylinder,
};

void EnsureModelFoldersScanned()
{
	if (g_modelFoldersScanned) {
		return;
	}
	g_modelFolders = Editor::ScanModelFolders();
	g_modelFoldersScanned = true;
}

// --- 名前まわり ---

bool NameExists(Object3dManager& manager, const std::string& name)
{
	for (Object3d* object : manager.GetAllObject()) {
		if (object && object->name_ == name) {
			return true;
		}
	}
	return false;
}

// 同名があれば "Name_1" "Name_2" … と後ろに足す。
// レンダラーもこの名前で登録するので、ここで一意にしておくと下流が全部楽になる
std::string MakeUniqueName(Object3dManager& manager, const std::string& desired)
{
	std::string base = desired.empty() ? std::string("Object") : desired;
	if (!NameExists(manager, base)) {
		return base;
	}
	for (int i = 1; i < 10000; ++i) {
		std::string candidate = base + "_" + std::to_string(i);
		if (!NameExists(manager, candidate)) {
			return candidate;
		}
	}
	return base;
}

bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle)
{
	if (needle.empty()) {
		return true;
	}
	auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
		[](char a, char b) {
			return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
		});
	return it != haystack.end();
}

// --- 生成の共通部分 ---

// Object3d を作って Object3dManager に登録し、生ポインタを返す。
// Object3d のコンストラクタが Initialize() まで済ませてくれる
Object3d* SpawnObject(Object3dManager& manager, const std::string& desiredName)
{
	const std::string name = MakeUniqueName(manager, desiredName);
	auto object = std::make_unique<Object3d>(name);
	Object3d* raw = object.get();
	manager.AddObject(std::move(object));
	return raw;
}

// レンダラーは RendererManager が所有し、Object3d は生ポインタで参照する。
// FindRender(name) で引き直すと同名衝突で事故るので、move する前にポインタを取っておく
void AttachRenderer(Object3d* object, std::unique_ptr<BaseRenderer> renderer)
{
	RendererManager* renderers = Editor::Ctx().rendererManager;
	if (!renderers || !object || !renderer) {
		return;
	}
	BaseRenderer* raw = renderer.get();
	renderers->AddRenderer(std::move(renderer));
	object->AddRenderer(raw);
}

Object3d* CreatePrimitiveObject(const std::string& desiredName, PrimitiveType type, const std::string& textureName)
{
	Object3dManager* manager = Editor::Ctx().object3dManager;
	if (!manager) {
		return nullptr;
	}
	Object3d* object = SpawnObject(*manager, desiredName);
	AttachRenderer(object, std::make_unique<PrimitiveRenderer>(object->name_, type, textureName));
	return object;
}

// --- 生成ダイアログ ---

void DrawCreatePopup()
{
	if (!ImGui::BeginPopup("##CreateObject")) {
		return;
	}

	ImGui::TextDisabled("新しい Object3d を作る");
	ImGui::Separator();

	ImGui::SetNextItemWidth(220.0f);
	ImGui::InputText("名前", g_newName, sizeof(g_newName));

	int kind = static_cast<int>(g_createKind);
	ImGui::RadioButton("モデル", &kind, 0);
	ImGui::SameLine();
	ImGui::RadioButton("プリミティブ", &kind, 1);
	ImGui::SameLine();
	ImGui::RadioButton("空", &kind, 2);
	g_createKind = static_cast<CreateKind>(kind);

	ImGui::Separator();

	bool canCreate = true;

	if (g_createKind == CreateKind::Model) {
		EnsureModelFoldersScanned();
		if (g_modelFolders.empty()) {
			ImGui::TextDisabled("Resource/Models にモデルがありません");
			canCreate = false;
		} else {
			g_selectedModelIndex = std::clamp(g_selectedModelIndex, 0, static_cast<int>(g_modelFolders.size()) - 1);
			ImGui::SetNextItemWidth(220.0f);
			// ラベルはImGuiのID元になる。上のラジオボタン("モデル")と衝突させないこと
			if (ImGui::BeginCombo("使うモデル", g_modelFolders[g_selectedModelIndex].c_str())) {
				for (int i = 0; i < static_cast<int>(g_modelFolders.size()); ++i) {
					const bool selected = (g_selectedModelIndex == i);
					if (ImGui::Selectable(g_modelFolders[i].c_str(), selected)) {
						g_selectedModelIndex = i;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
		if (ImGui::SmallButton("再スキャン")) {
			g_modelFoldersScanned = false;
		}
	} else if (g_createKind == CreateKind::Primitive) {
		ImGui::SetNextItemWidth(220.0f);
		ImGui::Combo("形", &g_primitiveIndex, kPrimitiveLabels, IM_ARRAYSIZE(kPrimitiveLabels));
		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputText("テクスチャ", g_primitiveTexture, sizeof(g_primitiveTexture));
		ImGui::TextDisabled("Resource/Images からのファイル名");
	} else {
		ImGui::TextDisabled("レンダラー無しで作ります");
	}

	ImGui::Separator();
	ImGui::SetNextItemWidth(220.0f);
	ImGui::DragFloat3("位置", g_newPosition, 0.05f);

	ImGui::Separator();

	ImGui::BeginDisabled(!canCreate);
	if (ImGui::Button("作成", ImVec2(100.0f, 0.0f))) {
		Object3d* created = nullptr;
		switch (g_createKind) {
		case CreateKind::Model:
			created = Editor::CreateModelObject(g_newName, g_modelFolders[g_selectedModelIndex]);
			break;
		case CreateKind::Primitive:
			created = CreatePrimitiveObject(g_newName, kPrimitiveTypes[g_primitiveIndex], g_primitiveTexture);
			break;
		case CreateKind::Empty:
			created = Editor::CreateEmptyObject(g_newName);
			break;
		}

		if (created) {
			created->GetWorldTransform()->GetTranslation() =
				Vector3(g_newPosition[0], g_newPosition[1], g_newPosition[2]);
			Editor::SelectObject(created->name_);
			EditorMenuBar::ShowToast("%s を作成しました", created->name_.c_str());
			ImGui::CloseCurrentPopup();
		}
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	if (ImGui::Button("閉じる", ImVec2(100.0f, 0.0f))) {
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}

// --- 一覧の1行 ---

// 複製元と同じ見た目のオブジェクトを作る。
// 今は「最初のレンダラーが参照しているモデル」だけを引き継ぐ（それ以外は空になる）
void DuplicateObject(Object3d* source)
{
	if (!source) {
		return;
	}
	std::string modelName;
	if (!source->GetRenderers().empty()) {
		modelName = Editor::FindLoadedModelName(source->GetRenderers().front()->GetModel());
	}

	Object3d* created = modelName.empty()
		? Editor::CreateEmptyObject(source->name_)
		: Editor::CreateModelObject(source->name_, modelName);
	if (!created) {
		return;
	}

	WorldTransform* from = source->GetWorldTransform();
	WorldTransform* to = created->GetWorldTransform();
	to->GetTranslation() = from->GetTranslation();
	to->GetRotation() = from->GetRotation();
	to->GetScale() = from->GetScale();
	created->GetOption() = source->GetOption();

	Editor::SelectObject(created->name_);
	EditorMenuBar::ShowToast("%s を複製しました", created->name_.c_str());
}

void DrawObjectRow(Object3d* object)
{
	ImGui::PushID(object);

	const bool selected = Editor::IsObjectSelected(object->name_);
	if (ImGui::Selectable(object->name_.c_str(), selected)) {
		Editor::SelectObject(object->name_);
	}

	if (ImGui::BeginPopupContextItem("##rowMenu")) {
		// 右クリックした行を選択状態にしてからメニューを出す
		Editor::SelectObject(object->name_);
		if (ImGui::MenuItem("複製")) {
			DuplicateObject(object);
		}
		if (ImGui::MenuItem("削除")) {
			if (Object3dManager* manager = Editor::Ctx().object3dManager) {
				manager->DeleteObject(object->name_);
				Editor::ClearObjectSelection();
				EditorMenuBar::ShowToast("%s を削除しました", object->name_.c_str());
			}
		}
		ImGui::EndPopup();
	}

	// 非表示のオブジェクトは灰色に落として、消えているのが設定のせいだと分かるようにする
	ImGui::SameLine();
	const ImVec4 dimmed = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
	ImGui::TextColored(dimmed, "  R:%zu C:%zu%s",
		object->GetRenderers().size(),
		object->GetColliders().size(),
		object->GetIsDraw() ? "" : "  (非表示)");

	ImGui::PopID();
}

} // namespace

Object3d* Editor::CreateEmptyObject(const std::string& desiredName)
{
	Object3dManager* manager = Ctx().object3dManager;
	if (!manager) {
		return nullptr;
	}
	return SpawnObject(*manager, desiredName);
}

Object3d* Editor::CreateModelObject(const std::string& desiredName, const std::string& modelName)
{
	Object3dManager* objects = Ctx().object3dManager;
	ModelManager* models = Ctx().modelManager;
	if (!objects || !models || modelName.empty()) {
		return nullptr;
	}

	// 未読み込みならここで読む。存在しない名前を渡すと ModelLoader の assert で落ちるので、
	// 呼び出し側は ScanModelFolders() の結果から選ぶこと
	models->LoadModel(modelName);

	Object3d* object = SpawnObject(*objects, desiredName);
	AttachRenderer(object, std::make_unique<ModelRenderer>(object->name_, modelName));
	return object;
}

void Editor::DrawHierarchyWindow()
{
	if (!EditorWindow::Begin("Hierarchy", EditorWindow::Category::kWorld)) {
		return;
	}

	Object3dManager* manager = Ctx().object3dManager;
	if (!manager) {
		ImGui::TextDisabled("EditorContext がまだ設定されていません");
		EditorWindow::End();
		return;
	}

	if (ImGui::Button("+ 作成")) {
		ImGui::OpenPopup("##CreateObject");
	}
	DrawCreatePopup();

	ImGui::SameLine();
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##filter", "名前で絞り込み", g_filter, sizeof(g_filter));

	const std::vector<Object3d*> objects = manager->GetAllObject();
	const std::string needle = g_filter;

	int shown = 0;
	ImGui::Separator();
	if (ImGui::BeginChild("##list", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()))) {
		for (Object3d* object : objects) {
			if (!object || !ContainsIgnoreCase(object->name_, needle)) {
				continue;
			}
			DrawObjectRow(object);
			++shown;
		}
		if (shown == 0) {
			ImGui::TextDisabled(objects.empty() ? "(オブジェクトがありません)" : "(該当なし)");
		}
	}
	ImGui::EndChild();

	ImGui::Separator();
	ImGui::TextDisabled("%d / %zu 件", shown, objects.size());

	EditorWindow::End();
}

#endif // _DEBUG
