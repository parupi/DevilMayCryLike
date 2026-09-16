#pragma once
#include <string>
#include "Math/Vector2.h"
#include "Math/Vector3.h"

class Enemy;

/// <summary>
/// ボスの演出（BossBreathEffect / BossAttackEffect）で共通に使う小物。
///   - 体のどこから出すか（口・爪元・翼の先）を決めるジョイント位置
///   - 画面効果の中心を合わせるための、ワールド座標 → 画面UV の変換
///   - ゲームカメラの揺れ・画角の蹴り
/// </summary>
namespace BossVfxUtil {

	/// <summary>
	/// ジョイントのワールド座標を取る。
	/// スキンモデルでない・そのジョイントが無い場合は false（呼び出し側で体基準の位置へ落とす）。
	/// **ポーズ更新（Enemy::Update）より後**に呼ぶこと
	/// </summary>
	/// <param name="rendererName">スキンモデルを描いているレンダラー名（＝ボスの名前）</param>
	bool TryGetJointWorldPosition(Enemy& enemy, const std::string& rendererName,
		const char* jointName, Vector3& outPosition);

	/// <summary>ジョイントのワールド座標。取れなければ fallback を返す</summary>
	Vector3 GetJointWorldPositionOr(Enemy& enemy, const std::string& rendererName,
		const char* jointName, const Vector3& fallback);

	/// <summary>ワールド座標を画面のUVへ。画面外なら false</summary>
	bool ToScreenUV(const Vector3& worldPosition, Vector2& outUV);

	/// <summary>
	/// ワールド座標と半径を、画面のUVと「画面の縦を1とした太さ」へ。
	/// カメラの後ろなら false（画面の外でも前にあれば true。帯の端として使えるように）
	/// </summary>
	bool ProjectToScreen(const Vector3& worldPosition, float worldRadius, Vector2& outUV, float& outRadius);

	/// <summary>カメラを揺らす（ゲームカメラでなければ何もしない）</summary>
	void AddCameraShake(float trauma);

	/// <summary>画角を一瞬広げる（ゲームカメラでなければ何もしない）</summary>
	void AddCameraFovPunch(float add);

} // namespace BossVfxUtil
