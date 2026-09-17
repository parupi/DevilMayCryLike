#pragma once
#ifdef _DEBUG

#include <optional>
#include <string>
#include "Math/Vector3.h"

class Object3d;

namespace Editor {

/// <summary>
/// シーン内の Object3d 一覧。選択・複製・削除と、新規オブジェクトの生成を行う。
/// Editor::Initialize で登録済み。
/// </summary>
void DrawHierarchyWindow();

/// <summary>
/// 空の Object3d を作って Object3dManager へ登録する。
/// 名前が既に使われていたら末尾に連番を足すので、戻り値の name_ を見ること。
/// EditorContext 未設定なら nullptr。
/// </summary>
Object3d* CreateEmptyObject(const std::string& desiredName);

/// <summary>
/// モデル付きの Object3d を作る。modelName は Resource/Models/&lt;name&gt;/ のフォルダ名。
/// モデルが未読み込みならここで読み込む。
/// </summary>
Object3d* CreateModelObject(const std::string& desiredName, const std::string& modelName);

/// <summary>
/// ステージデータのクラス名（"Ground" / "GruntMelee" / "PointLight" など）を指定して作る。
/// Object3dFactory に登録済みのものだけが対象で、未登録なら素の Object3d になる。
///
/// modelName は Ground / Prop のようにモデル名を使うクラスにだけ効く（空なら既定のまま）。
/// 作られたオブジェクトは保存対象（IsStageObject() == true）になる。
/// </summary>
/// <param name="colliderHalfExtents">
/// 指定するとOBBコライダーを付ける。Ground や敵は Initialize() / Update() で
/// コライダーを前提にしているので、生成時に付けておく必要がある
/// </param>
Object3d* CreateStageObject(const std::string& desiredName, const std::string& className,
	const std::string& modelName, const std::optional<Vector3>& colliderHalfExtents = std::nullopt);

} // namespace Editor

#endif // _DEBUG
