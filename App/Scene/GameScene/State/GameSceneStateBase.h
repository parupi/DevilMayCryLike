#pragma once

class GameScene;

class GameSceneStateBase
{
public:
	GameSceneStateBase() = default;
	~GameSceneStateBase() = default;

	virtual void Enter(GameScene& scene) = 0;
	virtual void Update(GameScene& scene) = 0;
	virtual void Exit(GameScene& scene) = 0;
};

