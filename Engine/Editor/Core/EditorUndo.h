#pragma once
#ifdef _DEBUG

#include <string>

class Object3d;

/// <summary>
/// エディタの取り消し／やり直し。今のところ**トランスフォームの変更だけ**を対象にする。
///
/// 置いたオブジェクトを少しずつ動かす作業で一番効くのがここで、
/// 生成・削除まで戻せるようにするにはオブジェクトを丸ごと復元する仕組み
/// （＝ステージデータへの書き出しと読み戻し）が要るため、そちらは持たせていない。
///
/// 使い方は「変える直前に Begin、変え終わったら End」の2点だけ:
///
/// <code>
/// if (ImGui::IsItemActivated())            EditorUndo::BeginTransformEdit(object);
/// if (ImGui::IsItemDeactivatedAfterEdit()) EditorUndo::EndTransformEdit(object);
/// </code>
///
/// **オブジェクトは名前で覚える**。ポインタは削除で消えるため
/// （EditorSelection と同じ理由）。取り消し時に見つからない履歴は捨てる。
/// </summary>
namespace EditorUndo {

/// <summary>変更前の値を控える。同じオブジェクトで二重に呼んでも最初の1回だけ効く</summary>
void BeginTransformEdit(Object3d* object);

/// <summary>変更を確定して履歴に積む。値が変わっていなければ何も積まない</summary>
void EndTransformEdit(Object3d* object);

/// <summary>Begin したけれど確定しないとき（ドラッグの取り消しなど）</summary>
void AbortTransformEdit();

bool CanUndo();
bool CanRedo();
void Undo();
void Redo();

/// <summary>履歴を捨てる。シーンを読み直したときに呼ぶ</summary>
void Clear();

/// <summary>直前の操作の説明。メニュー表示用（無ければ空文字）</summary>
const std::string& GetUndoLabel();
const std::string& GetRedoLabel();

/// <summary>Editメニューの中身</summary>
void DrawMenu();

/// <summary>Ctrl+Z / Ctrl+Y。EditorMenuBar から呼ぶ</summary>
void HandleShortcuts();

} // namespace EditorUndo

#endif // _DEBUG
