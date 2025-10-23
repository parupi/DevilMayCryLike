#pragma once
#include "math/Vector3.h"
#include <random>

/// <summary>
/// 攻撃ヒット時に発生する一時停止（ヒットストップ）を制御するクラス  
/// 一定時間ゲーム全体または対象の動きを止め、攻撃のインパクトを強調するために使用される
/// </summary>
class HitStop
{
private:
	/// <summary>
	/// ヒットストップに関するデータ構造体
	/// </summary>
	struct HitStopData {
		bool isActive = false; // ヒットストップがアクティブかどうか
		Vector3 translate{}; // ヒットストップ中の位置変化（揺れなどのエフェクト用）
	} hitStopData_;

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	HitStop() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~HitStop() = default;

	/// <summary>
	/// ヒットストップの更新処理  
	/// 時間経過に応じてヒットストップを解除したり、視覚効果を制御する
	/// </summary>
	void Update();

	/// <summary>
	/// ヒットストップを開始する  
	/// </summary>
	/// <param name="time">ヒットストップの継続時間</param>
	/// <param name="intensity">揺れの強さなど、ヒットストップの強度</param>
	void Start(float time, float intensity);

	/// <summary>
	/// 現在のヒットストップデータを取得する
	/// </summary>
	/// <returns>ヒットストップ情報構造体</returns>
	HitStopData GetHitStopData() const { return hitStopData_; }

private:
	// ヒットストップの強度
	float intensity_;
	// ヒットストップの最大時間
	float maxTime_;
	// 現在の経過時間
	float currentTimer_;
};
