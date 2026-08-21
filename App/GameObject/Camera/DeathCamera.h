#pragma once
#include "World3D/Camera/BaseCamera.h"
#include <Math/Vector3.h>
#include <random>

class Player;

/// <summary>
/// 死亡演出中のカメラ。倒れたプレイヤーへ寄りながら、
/// とどめの瞬間に強く揺れて収まっていく。
///
/// 世界の時間は死亡演出中ほぼ止まるので、進行には実時間（DeltaTime）を使う。
/// </summary>
class DeathCamera : public BaseCamera
{
public:
	DeathCamera(const std::string& cameraName, BaseCamera* sourceCamera, Player* player);
	~DeathCamera() override = default;

	/// <summary>
	/// プレイヤーを追従するカメラの更新処理
	/// 入力による回転・追従などの挙動を更新する
	/// </summary>
	void Update() override;

private:
	Player* player_ = nullptr;

	Vector3 basePos_;        // 死亡した瞬間のゲームカメラの位置

	float zoomTime_ = 0.0f;  // ズームの進行
	float totalTime_ = 2.0f; // ズーム演出全体の時間

	// === ランダムシェイク用 ===
	std::random_device seedGenerator_;
	std::mt19937 randomEngine_;

	// ── 調整パラメータ ──
	// とどめの瞬間がいちばん揺れて、寄りきるころには収まる
	static constexpr float kMaxShake = 0.25f;
	// 寄りきったときの水平距離[m]。近づけすぎると、とどめを刺した敵に画を塞がれる
	static constexpr float kEndDistance = 4.0f;
	// 寄りきったときの高さ[m]（プレイヤーからの相対）。上から見下ろす形にする
	static constexpr float kEndHeight = 2.6f;
	// 注視点をプレイヤーからどれだけ上に取るか[m]
	static constexpr float kLookHeight = 0.3f;
};
