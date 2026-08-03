#include "EditorSelection.h"
#ifdef _DEBUG

#include "EditorContext.h"
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Object3dManager.h"

namespace {
std::string g_selectedObject;
} // namespace

void Editor::SelectObject(const std::string& name)
{
	g_selectedObject = name;
}

void Editor::ClearObjectSelection()
{
	g_selectedObject.clear();
}

const std::string& Editor::GetSelectedObjectName()
{
	return g_selectedObject;
}

bool Editor::IsObjectSelected(const std::string& name)
{
	return !g_selectedObject.empty() && g_selectedObject == name;
}

Object3d* Editor::GetSelectedObject()
{
	if (g_selectedObject.empty()) {
		return nullptr;
	}
	Object3dManager* manager = Ctx().object3dManager;
	if (!manager) {
		return nullptr;
	}

	// Object3dManager::FindObject は見つからないとログを吐くので使わない。
	// ここは毎フレーム引き直す場所なので、消えた瞬間にログが溢れてしまう
	for (Object3d* object : manager->GetAllObject()) {
		if (object && object->name_ == g_selectedObject) {
			return object;
		}
	}
	return nullptr;
}

#endif // _DEBUG
