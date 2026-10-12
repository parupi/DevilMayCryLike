#pragma once
#include "Scene/GameScene/State/GameSceneStateBase.h"
class GameSceneStatePlay : public GameSceneStateBase
{
public:
	GameSceneStatePlay() = default;
	~GameSceneStatePlay() = default;

	void Enter(GameScene& scene) override;
	void Update(GameScene& scene) override;
	void Exit(GameScene& scene) override;
private:
	// 戦闘中かどうかでBGMを差し替える
	void UpdateBattleBGM(GameScene& scene);
	// チュートリアルの部屋で全項目を終えたら、少し間を置いてボス戦の部屋へ切り替える。
	// 切り替えを要求したら true
	bool UpdateTutorialFinish(GameScene& scene);
	// START（キーボードは M）を見て、ポーズを開くべきなら true を返す。
	// チュートリアルの部屋では長押しでスキップになり、その場でボス戦への遷移を始める
	bool UpdatePauseAndSkip(GameScene& scene);
	// ボス戦の部屋への切り替えを始める（二重には要求しない）
	void RequestBossBattle();

	enum class PlayState {
		Enter,
		Play,
	}state_ = PlayState::Enter;

	float muskAlpha_ = 0.0f;
	// チュートリアルを開始済みかどうか（メニューからの復帰時などに再発火させないため）
	bool tutorialStarted_ = false;

	// チュートリアルを終えてからの経過時間[s]。最後の項目が消えた余韻を少し見せてから切り替える
	float tutorialFinishTimer_ = 0.0f;
	// ボス戦への切り替えを要求済みか（暗転中に二重に要求しない・メニューを開かせない）
	bool bossTransitionRequested_ = false;
	static constexpr float kTutorialFinishDelay = 1.5f;

	// 死亡演出が始まってからの経過時間[s]。世界の時間を落としていくのに使う
	float deathWorldTimer_ = 0.0f;

	// とどめが入った瞬間に世界をこの速さまで落とし、kWorldStopDuration かけて止める。
	// プレイヤーだけは Player::Update が実時間で動くので、倒れる動きは止まらない
	static constexpr float kWorldSlowScale = 0.25f;
	static constexpr float kWorldStopDuration = 0.7f;
};

