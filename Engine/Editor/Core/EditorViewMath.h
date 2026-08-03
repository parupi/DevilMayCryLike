#pragma once
#ifdef _DEBUG

#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include <imgui/imgui.h>

/// <summary>
/// ゲームビューの絵の上に何かを重ねるための、投影とレイの計算。
///
/// ギズモ（EditorGizmo）とクリック選択（EditorPicking）が同じ前提を共有するのでここに置く。
/// エンジンは**行ベクトル・左手系**（`v * M`、平行移動は `m[3][*]`、深度は [0,1]）。
/// </summary>
namespace EditorView {

/// <summary>アクティブカメラと、画像がスクリーンのどこに出ているかの組</summary>
struct Context {
	Matrix4x4 viewProjection;
	Matrix4x4 inverseViewProjection;
	Vector3 cameraPosition{};
	Vector3 cameraRight{};
	Vector3 cameraUp{};
	Vector3 cameraForward{};
	ImVec2 imagePos{};  // 画像の左上のスクリーン座標
	ImVec2 imageSize{}; // 画像の表示サイズ
};

/// <summary>
/// アクティブカメラから Context を組む。
/// カメラが無い／行列が潰れている／サイズが不正なら false（このとき out は不定）。
/// </summary>
bool Build(Context& out, const ImVec2& imagePos, const ImVec2& imageSize);

/// <summary>ワールド座標を画像上のスクリーン座標へ。カメラの後ろなら false</summary>
bool WorldToScreen(const Context& view, const Vector3& world, ImVec2& outScreen);

/// <summary>
/// 画像上のスクリーン座標からワールドのレイを作る。
/// outDirection は正規化されるので、交差判定の t はそのままワールドでの距離になる。
/// </summary>
void ScreenToRay(const Context& view, const ImVec2& screen, Vector3& outOrigin, Vector3& outDirection);

/// <summary>ゼロ割りしない正規化。長さが無いときは fallback を返す</summary>
Vector3 SafeNormalize(const Vector3& v, const Vector3& fallback);

/// <summary>その点が画像の矩形の中にあるか</summary>
bool IsInsideImage(const Context& view, const ImVec2& screen);

} // namespace EditorView

#endif // _DEBUG
