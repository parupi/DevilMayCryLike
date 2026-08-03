#pragma once
#ifdef _DEBUG

#include <string>

class Object3d;

/// <summary>
/// Hierarchy と Inspector をつなぐ「今選んでいるもの」。
///
/// **ポインタではなく名前で持つ**。Object3d は削除されると
/// Object3dManager::RemoveDeadObject() で実体ごと消えるので、
/// ポインタを握っていると次のフレームにはぶら下がりになる。
/// 名前で持って毎フレーム引き直せば、消えたときは素直に nullptr が返る。
/// </summary>
namespace Editor {

/// <summary>オブジェクトを選択する（空文字なら選択解除）</summary>
void SelectObject(const std::string& name);

/// <summary>選択を解除する</summary>
void ClearObjectSelection();

/// <summary>選択中のオブジェクト名。未選択なら空文字</summary>
const std::string& GetSelectedObjectName();

/// <summary>その名前が今選択されているか</summary>
bool IsObjectSelected(const std::string& name);

/// <summary>
/// 選択中のオブジェクトを引き直す。
/// 未選択・既に削除済み・EditorContext 未設定のいずれでも nullptr を返す。
/// </summary>
Object3d* GetSelectedObject();

} // namespace Editor

#endif // _DEBUG
