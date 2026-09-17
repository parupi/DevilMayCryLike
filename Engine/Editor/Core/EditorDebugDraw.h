#pragma once
#ifdef _DEBUG

/// <summary>
/// デバッグ描画（ギズモ）の表示トグルを一箇所に集めたもの。
///
/// これまでは「コライダーを消したいのに、どのウィンドウにチェックがあるか分からない」
/// という状態だったので、描画する側が必ずここを見るようにしている。
/// 状態は Resource/GlobalVariables/Editor/EditorDebugDraw.json に保存される。
/// </summary>
namespace EditorDebugDraw {

enum class Flag {
	Collider,     // コライダーのワイヤー表示
	LightGizmo,   // ライト位置・範囲のギズモ
	AttackTrail,  // 攻撃の制御点とカーブ
	Grid,         // ワールドのグリッド
	Count,
};

/// <summary>そのデバッグ描画が有効か</summary>
bool IsEnabled(Flag flag);
/// <summary>有効／無効を設定する</summary>
void SetEnabled(Flag flag, bool enabled);

/// <summary>"Debug Draw" メニューの中身を描く（BeginMenu/EndMenu の内側で呼ぶ）</summary>
void DrawMenu();

/// <summary>
/// グリッドを描く。PrimitiveLineDrawer が使える描画パスの中から呼ぶこと。
/// Grid が無効なら何もしない。
/// </summary>
void DrawGrid();

/// <summary>保存された状態を読み込む。起動時に一度だけ呼ぶ</summary>
void LoadSettings();
/// <summary>現在の状態を保存する</summary>
void SaveSettings();

} // namespace EditorDebugDraw

#endif // _DEBUG
