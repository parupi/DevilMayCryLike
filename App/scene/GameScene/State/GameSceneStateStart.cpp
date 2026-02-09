#include "GameSceneStateStart.h"
#include "scene/GameScene/GameScene.h"

void GameSceneStateStart::Enter(GameScene& scene)
{
	stageStart_ = std::make_unique<StageStart>();
	stageStart_->Initialize();
	scene.GetInputContext()->SetCanPlayerMove(false);
}

void GameSceneStateStart::Update(GameScene& scene)
{
	stageStart_->Update();

	if (stageStart_->IsComplete()) {
		scene.ChangeState("Play");
	}
}

void GameSceneStateStart::Exit(GameScene& scene)
{

}
