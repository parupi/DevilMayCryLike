#include "EditorUndo.h"
#ifdef _DEBUG

#include "EditorContext.h"
#include "EditorSelection.h"

#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/WorldTransform.h"

#include <imgui/imgui.h>
#include <vector>

namespace {

struct TransformValues {
	Vector3 translate{};
	Quaternion rotate{};
	Vector3 scale{ 1.0f, 1.0f, 1.0f };
};

struct TransformEntry {
	std::string objectName;
	TransformValues before;
	TransformValues after;
	std::string label;
};

// 際限なく貯めるとメモリを食うので上限を決めておく。
// トランスフォームだけなので1件あたりは小さい
constexpr size_t kMaxHistory = 256;

std::vector<TransformEntry> g_undoStack;
std::vector<TransformEntry> g_redoStack;

// Begin と End の間だけ持つ、編集中の控え
bool g_editing = false;
std::string g_editingName;
TransformValues g_editingBefore;

const std::string kEmptyLabel;

TransformValues Read(Object3d* object)
{
	TransformValues values;
	WorldTransform* transform = object->GetWorldTransform();
	values.translate = transform->GetTranslation();
	values.rotate = transform->GetRotation();
	values.scale = transform->GetScale();
	return values;
}

void Write(Object3d* object, const TransformValues& values)
{
	WorldTransform* transform = object->GetWorldTransform();
	transform->GetTranslation() = values.translate;
	transform->GetRotation() = values.rotate;
	transform->GetScale() = values.scale;
}

bool IsSame(const TransformValues& a, const TransformValues& b)
{
	return a.translate == b.translate && a.rotate == b.rotate && a.scale == b.scale;
}

// 名前で引き直す。FindObject は見つからないとログを吐くので使わない
Object3d* FindByName(const std::string& name)
{
	Object3dManager* manager = Editor::Ctx().object3dManager;
	if (!manager || name.empty()) {
		return nullptr;
	}
	for (Object3d* object : manager->GetAllObject()) {
		if (object && object->isAlive && object->name_ == name) {
			return object;
		}
	}
	return nullptr;
}

void Push(std::vector<TransformEntry>& stack, TransformEntry entry)
{
	stack.push_back(std::move(entry));
	if (stack.size() > kMaxHistory) {
		stack.erase(stack.begin());
	}
}

// 履歴を1件取り出して適用する。対象が消えていたら捨てて次を試す
bool Apply(std::vector<TransformEntry>& from, std::vector<TransformEntry>& to, bool useBefore)
{
	while (!from.empty()) {
		TransformEntry entry = from.back();
		from.pop_back();

		Object3d* object = FindByName(entry.objectName);
		if (!object) {
			continue; // 対象が消えている履歴は無効
		}
		Write(object, useBefore ? entry.before : entry.after);
		Editor::SelectObject(entry.objectName);
		Push(to, std::move(entry));
		return true;
	}
	return false;
}

} // namespace

void EditorUndo::BeginTransformEdit(Object3d* object)
{
	if (!object || g_editing) {
		return;
	}
	g_editing = true;
	g_editingName = object->name_;
	g_editingBefore = Read(object);
}

void EditorUndo::EndTransformEdit(Object3d* object)
{
	if (!g_editing) {
		return;
	}
	g_editing = false;

	// 別のオブジェクトで End されたら、対応が取れないので積まない
	if (!object || object->name_ != g_editingName) {
		return;
	}
	const TransformValues after = Read(object);
	if (IsSame(g_editingBefore, after)) {
		return; // 触っただけで値が変わっていない
	}

	TransformEntry entry;
	entry.objectName = g_editingName;
	entry.before = g_editingBefore;
	entry.after = after;
	entry.label = g_editingName + " のトランスフォーム";

	Push(g_undoStack, std::move(entry));
	// 新しい操作をしたらやり直しの履歴は無効になる
	g_redoStack.clear();
}

void EditorUndo::AbortTransformEdit()
{
	g_editing = false;
}

bool EditorUndo::CanUndo() { return !g_undoStack.empty(); }
bool EditorUndo::CanRedo() { return !g_redoStack.empty(); }

void EditorUndo::Undo()
{
	Apply(g_undoStack, g_redoStack, true);
}

void EditorUndo::Redo()
{
	Apply(g_redoStack, g_undoStack, false);
}

void EditorUndo::Clear()
{
	g_undoStack.clear();
	g_redoStack.clear();
	g_editing = false;
}

const std::string& EditorUndo::GetUndoLabel()
{
	return g_undoStack.empty() ? kEmptyLabel : g_undoStack.back().label;
}

const std::string& EditorUndo::GetRedoLabel()
{
	return g_redoStack.empty() ? kEmptyLabel : g_redoStack.back().label;
}

void EditorUndo::DrawMenu()
{
	if (ImGui::MenuItem("取り消し", "Ctrl+Z", false, CanUndo())) {
		Undo();
	}
	if (ImGui::MenuItem("やり直し", "Ctrl+Y", false, CanRedo())) {
		Redo();
	}
	ImGui::Separator();
	ImGui::TextDisabled("取り消せるのはトランスフォームの変更だけです");
	if (CanUndo()) {
		ImGui::TextDisabled("次に戻すもの: %s", GetUndoLabel().c_str());
	}
	ImGui::TextDisabled("履歴: 取り消し %zu / やり直し %zu", g_undoStack.size(), g_redoStack.size());
	if (ImGui::MenuItem("履歴を捨てる", nullptr, false, CanUndo() || CanRedo())) {
		Clear();
	}
}

void EditorUndo::HandleShortcuts()
{
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal)) {
		Undo();
	}
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, ImGuiInputFlags_RouteGlobal)) {
		Redo();
	}
}

#endif // _DEBUG
