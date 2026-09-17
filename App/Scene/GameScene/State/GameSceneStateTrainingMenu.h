#pragma once
#include "Scene/GameScene/State/GameSceneStateBase.h"

/// <summary>
/// トレーニングルームの設定メニューを開いている状態。
///
/// ポーズ（GameSceneStateMenu）と違い、暗転もBGMの停止も挟まない。
/// 設定を変えてすぐ戦いに戻る場所なので、パネル自身の暗幕だけで済ませている。
/// 生成されるのはトレーニングのときだけ（本編では誰もここへ遷移してこない）
/// </summary>
class GameSceneStateTrainingMenu : public GameSceneStateBase
{
public:
	GameSceneStateTrainingMenu() = default;
	~GameSceneStateTrainingMenu() = default;

	void Enter(GameScene& scene) override;
	void Update(GameScene& scene) override;
	void Exit(GameScene& scene) override;
};
