#pragma once
#ifdef _DEBUG

#include "Math/Vector3.h"
#include <imgui/imgui.h>

class Object3d;

/// <summary>
/// ゲームビューの絵を直接クリックしてオブジェクトを選ぶ。
///
/// マウス位置からワールドのレイを飛ばし、描画中の Object3d のメッシュと交差判定する。
/// 選ばれたものは EditorSelection に入るので、Hierarchy / Inspector / ギズモが
/// そのまま追従する。
///
/// 判定に使うのはモデルのCPU側の頂点で、モデルごとに1度だけ三角形リストとAABBに
/// 焼いてキャッシュする（`Editor::Finalize()` で捨てる）。
/// スキンモデルだけは頂点がバインドポーズのままなので、AABBだけで判定する。
///
/// 呼び出しは EditorGameView::DrawWindow() の **EditorGizmo::DrawOverlay の直後**。
/// ギズモが掴んでいるクリックは奪わない。
/// </summary>
namespace EditorPicking {

/// <summary>クリックで選択できるか</summary>
bool IsEnabled();
void SetEnabled(bool enabled);

/// <summary>選択中のオブジェクトを枠で囲うか</summary>
bool IsOutlineEnabled();
void SetOutlineEnabled(bool enabled);

/// <summary>
/// レイと交差する一番手前のオブジェクトを返す。無ければ nullptr。
/// outDistance にはレイ原点からの距離が入る。
/// </summary>
Object3d* Raycast(const Vector3& rayOrigin, const Vector3& rayDirection, float* outDistance = nullptr);

/// <summary>
/// ゲームビューのクリックを処理し、選択中のオブジェクトを枠で囲う。
/// EditorGizmo::DrawOverlay の直後、同じウィンドウの中から呼ぶこと。
/// </summary>
void HandleGameView(const ImVec2& imagePos, const ImVec2& imageSize);

/// <summary>Gizmoメニューの中の「クリック選択」の部分を描く</summary>
void DrawMenu();

/// <summary>焼いたメッシュのキャッシュを捨てる</summary>
void ClearCache();

void LoadSettings();
void SaveSettings();

} // namespace EditorPicking

#endif // _DEBUG
