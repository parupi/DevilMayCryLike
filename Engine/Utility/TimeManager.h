#pragma once

/// <summary>
/// ゲーム内の時間を用途ごとのチャンネルに分けて配るクラス。
///
/// ヒットストップ中に「ゲームは止まるが、VFXは少しだけ動き、UIは通常どおり動く」
/// といった演出を成立させるために、実時間を用途別のデルタタイムへ変換して提供する。
///
/// <code>
/// Real  : 実時間。常にそのまま
/// Game  : 実時間 * gameTimeScale        （ヒットストップで止まる）
/// VFX   : 実時間 * lerp(gameScale, 1, bias)（止まっている間もbias分だけ動く）
/// UI    : 実時間。ヒットストップの影響を受けない
/// </code>
///
/// 使い方:
///  1. 毎フレーム DeltaTime::Update() の直後に Update() を呼ぶ（要求値がリセットされる）
///  2. ヒットストップを持つ側が毎フレーム RequestGameTimeScale() で希望のスケールを申告する
///  3. 時間を必要とする側が GetGameDelta() / GetVFXDelta() / GetUIDelta() を読む
///
/// 2 は複数箇所（プレイヤーと各敵）から呼ばれるため、最も遅いスケールが採用される。
/// </summary>
class TimeManager
{
public:
	/// <summary>
	/// 実時間を取り込み、このフレームのタイムスケール要求を受け付ける状態に戻す。
	/// RequestGameTimeScale() を呼ぶすべての更新処理より前に呼ぶこと。
	/// </summary>
	static void Update();

	/// <summary>
	/// このフレームのゲーム時間スケールを要求する（0.0f = 完全停止、1.0f = 通常速度）。
	/// 複数から要求された場合は最も遅いもの（最小値）が採用される。
	/// </summary>
	static void RequestGameTimeScale(float scale);

	/// <summary>このフレームで採用されているゲーム時間スケール</summary>
	static float GetGameTimeScale() { return gameTimeScale_; }
	/// <summary>VFX用の時間スケール（ヒットストップ中も vfxBias 分だけ進む）</summary>
	static float GetVFXTimeScale();

	/// <summary>実時間[秒]。ヒットストップの影響を受けない</summary>
	static float GetRealDelta() { return realDelta_; }
	/// <summary>ゲーム進行用[秒]。ヒットストップで止まる</summary>
	static float GetGameDelta() { return realDelta_ * gameTimeScale_; }
	/// <summary>VFX用[秒]。ヒットストップ中も少しだけ進む</summary>
	static float GetVFXDelta() { return realDelta_ * GetVFXTimeScale(); }
	/// <summary>UI用[秒]。常に実時間で進む</summary>
	static float GetUIDelta() { return realDelta_; }

	/// <summary>
	/// ヒットストップ中にVFXをどれだけ動かすかの係数[0,1]。
	/// 0 = ゲームと完全に一緒に止まる / 1 = ヒットストップを一切受けない
	/// </summary>
	static void SetVFXBias(float bias);
	static float GetVFXBias() { return vfxBias_; }

private:
	// 実時間の1フレーム経過秒数
	static float realDelta_;
	// このフレームのゲーム時間スケール（Update()で1.0に戻り、Requestで下がる）
	static float gameTimeScale_;
	// ヒットストップ中にVFXを動かす割合
	static float vfxBias_;
};
