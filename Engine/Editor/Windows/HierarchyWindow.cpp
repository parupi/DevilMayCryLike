#include "HierarchyWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorAssetUtil.h"
#include "Editor/Core/EditorHost.h"
#include "Editor/Core/EditorMenuBar.h"
#include "Editor/Core/EditorSelection.h"

#include "Scene/Object3dFactory.h"
#include "World3D/Collider/CollisionManager.h"
#include "World3D/Collider/OBBCollider.h"
#include "World3D/Collider/SphereCollider.h"
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

// ステージデータのクラス（Object3dFactory の登録名）
int g_selectedClassIndex = 0;
// コライダー。Ground や敵はこれが無いと成立しないので既定で付ける。
// サイズはフルサイズで持つ（2.0 = 半分が 1.0）
bool g_addCollider = true;
float g_colliderSize[3] = { 2.0f, 2.0f, 2.0f };

char g_filter[64] = "";

// 「並べて複製」の設定。壁を横一列に並べる用途が多いので既定はX方向
float g_arrayStep[3] = { 2.0f, 0.0f, 0.0f };
int g_arrayCount = 3;

// 素の Object3d。このクラスのときだけモデル / プリミティブ / 空 を選べる
const char* const kPlainObjectClass = "Object3d";

const char* const kPrimitiveLabels[] = { "Plane", "Ring", "Cylinder" };
constexpr PrimitiveType kPrimitiveTypes[] = {
	PrimitiveType::Plane, PrimitiveType::Ring, PrimitiveType::Cylinder,
};

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
	// エディタで作ったものはステージデータに保存する
	raw->SetStageObject(true);
	manager.AddObject(std::move(object));
	return raw;
}

// OBBコライダーを付ける。所有は CollisionManager、Object3d は生ポインタで参照する
void AttachBoxCollider(Object3d* object, const Vector3& halfExtents)
{
	if (!object) {
		return;
	}
	auto collider = std::make_unique<OBBCollider>(object->name_);
	OBBData data;
	data.halfExtents = halfExtents;
	collider->GetColliderData() = data;

	BaseCollider* raw = collider.get();
	CollisionManager::GetInstance().AddCollider(std::move(collider));
	object->AddCollider(raw);
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

	ImGui::TextDisabled("ステージに置くオブジェクトを作る");
	ImGui::Separator();

	ImGui::SetNextItemWidth(220.0f);
	ImGui::InputText("名前", g_newName, sizeof(g_newName));

	// --- クラス（ステージデータの "class"）---
	const std::vector<std::string> classNames = Object3dFactory::GetClassNames();
	if (classNames.empty()) {
		ImGui::TextDisabled("Object3dFactory にクラスが登録されていません");
		ImGui::EndPopup();
		return;
	}
	g_selectedClassIndex = std::clamp(g_selectedClassIndex, 0, static_cast<int>(classNames.size()) - 1);

	ImGui::SetNextItemWidth(220.0f);
	if (ImGui::BeginCombo("クラス", classNames[g_selectedClassIndex].c_str())) {
		for (int i = 0; i < static_cast<int>(classNames.size()); ++i) {
			const bool selected = (g_selectedClassIndex == i);
			if (ImGui::Selectable(classNames[i].c_str(), selected)) {
				g_selectedClassIndex = i;
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	const std::string& className = classNames[g_selectedClassIndex];
	const bool isPlainObject = (className == kPlainObjectClass);
	const bool usesModelName = Object3dFactory::UsesModelName(className);

	ImGui::Separator();

	bool canCreate = true;

	// 素の Object3d だけは中身（モデル / プリミティブ / 空）を選べる
	if (isPlainObject) {
		int kind = static_cast<int>(g_createKind);
		ImGui::RadioButton("モデル", &kind, 0);
		ImGui::SameLine();
		ImGui::RadioButton("プリミティブ", &kind, 1);
		ImGui::SameLine();
		ImGui::RadioButton("空", &kind, 2);
		g_createKind = static_cast<CreateKind>(kind);
	}

	// モデルを選ばせるのは「素のObject3d＋モデル」か、モデル名を使うクラス（Ground / Prop）のとき
	const bool needsModelPicker = (isPlainObject && g_createKind == CreateKind::Model) || usesModelName;

	const std::vector<std::string>& modelFolders = Editor::CachedModelFolders();

	if (needsModelPicker) {
		if (modelFolders.empty()) {
			ImGui::TextDisabled("Resource/Models にモデルがありません");
			canCreate = false;
		} else {
			g_selectedModelIndex = std::clamp(g_selectedModelIndex, 0, static_cast<int>(modelFolders.size()) - 1);
			ImGui::SetNextItemWidth(220.0f);
			// ラベルはImGuiのID元になる。上のラジオボタン("モデル")と衝突させないこと
			if (ImGui::BeginCombo("使うモデル", modelFolders[g_selectedModelIndex].c_str())) {
				for (int i = 0; i < static_cast<int>(modelFolders.size()); ++i) {
					const bool selected = (g_selectedModelIndex == i);
					if (ImGui::Selectable(modelFolders[i].c_str(), selected)) {
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
			Editor::CachedModelFolders(true);
		}
	} else if (isPlainObject && g_createKind == CreateKind::Primitive) {
		ImGui::SetNextItemWidth(220.0f);
		ImGui::Combo("形", &g_primitiveIndex, kPrimitiveLabels, IM_ARRAYSIZE(kPrimitiveLabels));
		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputText("テクスチャ", g_primitiveTexture, sizeof(g_primitiveTexture));
		ImGui::TextDisabled("Resource/Images からのファイル名");
	} else if (isPlainObject) {
		ImGui::TextDisabled("レンダラー無しで作ります");
	} else {
		ImGui::TextDisabled("見た目は %s クラスが決めます", className.c_str());
	}

	// --- コライダー ---
	ImGui::Separator();
	ImGui::Checkbox("コライダーを付ける", &g_addCollider);
	if (g_addCollider) {
		ImGui::SetNextItemWidth(220.0f);
		ImGui::DragFloat3("サイズ", g_colliderSize, 0.05f, 0.0f, 1000.0f);
	} else if (!isPlainObject) {
		ImGui::TextDisabled("※ Ground や敵はコライダーが前提です");
	}

	ImGui::Separator();
	ImGui::SetNextItemWidth(220.0f);
	ImGui::DragFloat3("位置", g_newPosition, 0.05f);

	ImGui::Separator();

	ImGui::BeginDisabled(!canCreate);
	if (ImGui::Button("作成", ImVec2(100.0f, 0.0f))) {
		// サイズはフルサイズで入力させ、コライダーには半分を渡す
		const std::optional<Vector3> halfExtents = g_addCollider
			? std::optional<Vector3>(Vector3(g_colliderSize[0], g_colliderSize[1], g_colliderSize[2]) * 0.5f)
			: std::nullopt;

		Object3d* created = nullptr;
		if (isPlainObject) {
			switch (g_createKind) {
			case CreateKind::Model:
				created = Editor::CreateModelObject(g_newName, modelFolders[g_selectedModelIndex]);
				break;
			case CreateKind::Primitive:
				created = CreatePrimitiveObject(g_newName, kPrimitiveTypes[g_primitiveIndex], g_primitiveTexture);
				break;
			case CreateKind::Empty:
				created = Editor::CreateEmptyObject(g_newName);
				break;
			}
			// 素の Object3d は Initialize() をコンストラクタで済ませているので後付けでよい
			if (created && halfExtents) {
				AttachBoxCollider(created, halfExtents.value());
			}
		} else {
			const std::string modelName = (usesModelName && !modelFolders.empty())
				? modelFolders[g_selectedModelIndex]
				: std::string();
			created = Editor::CreateStageObject(g_newName, className, modelName, halfExtents);
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

// 複製元と同じオブジェクトを作って返す。
// クラス・モデル名・OBBコライダー・トランスフォーム・描画設定を引き継ぎ、
// クラス固有の設定（ライトやイベントの対象）は App の複製ハンドラに任せる
Object3d* DuplicateObject(Object3d* source)
{
	if (!source) {
		return nullptr;
	}

	// Ground / Prop はクラスがモデル名を持っている。
	// 素の Object3d はレンダラーが参照しているモデルから逆引きする
	std::string modelName = source->GetModelName();
	if (modelName.empty() && !source->GetRenderers().empty()) {
		modelName = Editor::FindLoadedModelName(source->GetRenderers().front()->GetModel());
	}

	std::optional<Vector3> halfExtents;
	for (BaseCollider* collider : source->GetColliders()) {
		if (auto* obb = dynamic_cast<OBBCollider*>(collider)) {
			halfExtents = obb->GetColliderData().halfExtents;
			break;
		}
	}

	Object3d* created = nullptr;
	if (source->GetClassName() == kPlainObjectClass) {
		created = modelName.empty()
			? Editor::CreateEmptyObject(source->name_)
			: Editor::CreateModelObject(source->name_, modelName);
		if (created && halfExtents) {
			AttachBoxCollider(created, halfExtents.value());
		}
	} else {
		created = Editor::CreateStageObject(source->name_, source->GetClassName(), modelName, halfExtents);
	}
	if (!created) {
		return nullptr;
	}

	WorldTransform* from = source->GetWorldTransform();
	WorldTransform* to = created->GetWorldTransform();
	to->GetTranslation() = from->GetTranslation();
	to->GetRotation() = from->GetRotation();
	to->GetScale() = from->GetScale();
	created->GetOption() = source->GetOption();
	created->SetIsDraw(source->GetIsDraw());

	// 球コライダーは CreateStageObject が扱わないので、ここで足す
	if (!halfExtents) {
		for (BaseCollider* collider : source->GetColliders()) {
			if (auto* sphere = dynamic_cast<SphereCollider*>(collider)) {
				auto copy = std::make_unique<SphereCollider>(created->name_);
				copy->GetColliderData() = sphere->GetColliderData();
				BaseCollider* raw = copy.get();
				CollisionManager::GetInstance().AddCollider(std::move(copy));
				created->AddCollider(raw);
				break;
			}
		}
	}

	// ライトやイベントの対象など、クラス固有の設定を App にコピーしてもらう
	Editor::NotifyObjectDuplicated(source, created);
	return created;
}

// 一定の間隔でまとめて複製する。モジュラーな壁や床を並べる用
void DuplicateArray(Object3d* source, const Vector3& step, int count)
{
	if (!source || count <= 0) {
		return;
	}
	// 複製元は動かさず、1個ずつずらして置いていく
	const Vector3 origin = source->GetWorldTransform()->GetTranslation();
	Object3d* last = nullptr;
	for (int i = 1; i <= count; ++i) {
		Object3d* created = DuplicateObject(source);
		if (!created) {
			break;
		}
		created->GetWorldTransform()->GetTranslation() = origin + step * static_cast<float>(i);
		last = created;
	}
	if (last) {
		Editor::SelectObject(last->name_);
		EditorMenuBar::ShowToast("%d 個並べて複製しました", count);
	}
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
			if (Object3d* created = DuplicateObject(object)) {
				Editor::SelectObject(created->name_);
				EditorMenuBar::ShowToast("%s を複製しました", created->name_.c_str());
			}
		}
		if (ImGui::BeginMenu("並べて複製")) {
			ImGui::SetNextItemWidth(200.0f);
			ImGui::DragFloat3("間隔", g_arrayStep, 0.05f);
			ImGui::SetNextItemWidth(200.0f);
			ImGui::SliderInt("個数", &g_arrayCount, 1, 50);
			if (ImGui::Button("並べる", ImVec2(120.0f, 0.0f))) {
				DuplicateArray(object, Vector3(g_arrayStep[0], g_arrayStep[1], g_arrayStep[2]), g_arrayCount);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndMenu();
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
	ImGui::TextColored(dimmed, "  [%s] R:%zu C:%zu%s%s",
		object->GetClassName().c_str(),
		object->GetRenderers().size(),
		object->GetColliders().size(),
		object->GetIsDraw() ? "" : "  (非表示)",
		object->IsStageObject() ? "" : "  (保存対象外)");

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
	// 保存して読み直したときに同じモデルが復元されるよう、名前も持たせておく
	object->SetModelName(modelName);
	AttachRenderer(object, std::make_unique<ModelRenderer>(object->name_, modelName));
	return object;
}

Object3d* Editor::CreateStageObject(const std::string& desiredName, const std::string& className,
	const std::string& modelName, const std::optional<Vector3>& colliderHalfExtents)
{
	Object3dManager* objects = Ctx().object3dManager;
	if (!objects) {
		return nullptr;
	}

	const std::string name = MakeUniqueName(*objects, desiredName);
	std::unique_ptr<Object3d> object = Object3dFactory::Create(className, name);
	Object3d* raw = object.get();
	raw->SetStageObject(true);

	// モデル名は Initialize() より前に渡す（Ground / Prop はここで見たものを読み込む）。
	// 存在しない名前は ModelLoader の assert に落ちるので、
	// 呼び出し側は ScanModelFolders() の結果から選ぶこと
	if (!modelName.empty()) {
		raw->SetModelName(modelName);
	}

	// コライダーも Initialize() より前に付ける。Ground は Initialize() の中で
	// コライダーのカテゴリを設定するため、後から付けると Ground 扱いにならない
	if (colliderHalfExtents) {
		AttachBoxCollider(raw, colliderHalfExtents.value());
	}

	raw->Initialize();

	objects->AddObject(std::move(object));
	return raw;
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
