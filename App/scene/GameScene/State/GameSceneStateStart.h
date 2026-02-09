#pragma once
#include "GameSceneStateBase.h"
#include "GameObject/UI/StageStart/StageStart.h"
#include <memory>
class GameSceneStateStart : public GameSceneStateBase
{
public:
	GameSceneStateStart() = default;
	~GameSceneStateStart() = default;

	void Enter(GameScene& scene) override;
	void Update(GameScene& scene) override;
	void Exit(GameScene& scene) override;

private:
	std::unique_ptr<StageStart> stageStart_; ///< ステージ開始時のカメラ演出
};

