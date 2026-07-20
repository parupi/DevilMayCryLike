#pragma once
#include "BaseEvent.h"
#include <string>

class Enemy;
class Player;

/// <summary>
/// ボス出現イベント。
/// プレイヤーがトリガー（BOXコライダー）に進入すると発動し、
///  ・カメラをボスのアップに切り替える
///  ・黒い粒子が収束しながらボスがゆっくりディゾルブで出現する
///  ・出現完了後、カメラをゲームカメラへ戻す
/// 演出中はプレイヤーをその場に固定する。
///
/// レベルエディタからは "Event_BossSpawn" として配置し、
/// 対象のボス（シーン内の敵オブジェクト）をパネルで設定する。
/// トリガー用に BOX コライダーが必須。
/// </summary>
class BossSpawnEvent : public BaseEvent {
public:
	BossSpawnEvent(std::string objectName);
	virtual ~BossSpawnEvent() override = default;

	void Initialize() override;
	void Update(float deltaTime) override;

	/// <summary>イベント発動：カメラ切り替え＋ボス出現開始</summary>
	void Execute() override;

	/// <summary>プレイヤー進入で発動する（トリガー）</summary>
	void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;

	/// <summary>出現させるボスのオブジェクト名を設定する（SceneBuilderから）</summary>
	void SetBossName(const std::string& name) { bossName_ = name; }

private:
	enum class Phase {
		Idle,     // 未発動
		Playing,  // 演出再生中
		Finished, // 終了
	};

	/// <summary>ボスを名前から解決する（削除済みなら nullptr）</summary>
	Enemy* FindBoss() const;

	/// <summary>演出を終了してゲームカメラへ戻す</summary>
	void EndCutscene();

	std::string bossName_;
	Player* player_ = nullptr;

	Phase phase_ = Phase::Idle;
	float timer_ = 0.0f;

	int skipFrames_ = 30;
	int currentFrame_ = 0;

	// ── 調整パラメータ ──
	static constexpr float kAppearDuration = 3.5f;    // ボスのディゾルブ出現時間[s]（通常の敵よりゆっくり）
	static constexpr int kAppearEmitCount = 6;        // 収束粒子の密度（通常の敵より濃く）
	static constexpr float kHoldTime = 0.8f;          // 出現完了後にボスを見せておく時間[s]
	static constexpr float kMaxCutsceneTime = 8.0f;   // 演出の最大時間[s]（保険）
	static constexpr float kCameraDistance = 8.5f;    // ボスからカメラまでの距離[m]
	static constexpr float kCameraSideOffset = 1.8f;  // カメラの横ずらし[m]（構図用）
	static constexpr float kCameraHeight = 2.5f;      // カメラの高さオフセット[m]
	static constexpr float kLookAtHeight = 1.8f;      // 注視点の高さ（ボスの上半身）[m]
	static constexpr float kCameraInTransition = 0.5f;  // 演出カメラへの切り替え時間[s]
	static constexpr float kCameraOutTransition = 1.2f; // ゲームカメラへ戻る時間[s]
};
