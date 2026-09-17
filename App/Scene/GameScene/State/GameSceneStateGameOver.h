#pragma once
#include "Scene/GameScene/State/GameSceneStateBase.h"

/// <summary>
/// 死亡後の状態。やり直すかタイトルへ戻るかを選ばせる。
///
/// 以前は PlayerStateDeath が直接 GAMEPLAY を読み直していたので、
/// 死ぬと問答無用でステージ頭から再開されていた
/// </summary>
class GameSceneStateGameOver : public GameSceneStateBase
{
public:
	GameSceneStateGameOver() = default;
	~GameSceneStateGameOver() = default;

	void Enter(GameScene& scene) override;
	void Update(GameScene& scene) override;
	void Exit(GameScene& scene) override;

private:
	/// <summary>シーンの切り替えを要求済みか。暗転中に二重で要求しないための見張り</summary>
	bool requested_ = false;
};
