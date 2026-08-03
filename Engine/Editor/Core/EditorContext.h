#pragma once
#ifdef _DEBUG

#include "Core/EngineContext.h"

/// <summary>
/// エディタからエンジンの各サービスを引くための窓口。
///
/// エディタのウィンドウは、必要なマネージャを直接 GetInstance() で掴まず
/// ここを経由して取る。そうしておくと「エディタがエンジンのどこに触っているか」が
/// EngineContext の1枚を見るだけで分かる。
///
/// <code>
/// if (auto* objects = Editor::Ctx().object3dManager) {
///     for (Object3d* obj : objects->GetAllObject()) { ... }
/// }
/// </code>
///
/// 注意: ImGuiManager::Initialize() の時点ではまだ中身が入っていない
/// （MyGameTitle が全サービスを生成し終えてから SetContext される）。
/// ウィンドウ側は必ずポインタの null チェックをすること。
/// </summary>
namespace Editor {

/// <summary>エンジン側のサービス一覧を渡す。MyGameTitle::Initialize() から一度だけ呼ぶ</summary>
void SetContext(const EngineContext& context);

/// <summary>SetContext が済んでいるか</summary>
bool HasContext();

/// <summary>
/// サービス一覧を取る。未設定でも全メンバ nullptr の EngineContext が返るので、
/// 各ポインタを見てから使えば落ちない。
/// </summary>
const EngineContext& Ctx();

} // namespace Editor

#endif // _DEBUG
