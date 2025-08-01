#include "SceneManager.h"
#include <cassert>

SceneManager* SceneManager::instance = nullptr;
std::once_flag SceneManager::initInstanceFlag;

SceneManager* SceneManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new SceneManager();
		});
	return instance;
}

void SceneManager::Finalize()
{
	// 最後のシーンの終了と解放
	scene_->Finalize();
	delete scene_;
	scene_ = nullptr;

	delete instance;
	instance = nullptr;
}

void SceneManager::Update()
{
	// 次シーンの予約があるなら
	if (nextScene_) {
		// 旧シーンの終了
		if (scene_) {
			scene_->Finalize();
			delete scene_;
		}

		// シーンの切り替え
		scene_ = nextScene_;
		nextScene_ = nullptr;

		scene_->SetSceneManager(this);

		// 次シーンを初期化する
		scene_->Initialize();
	}

	scene_->Update();
}

void SceneManager::Draw()
{
	scene_->Draw();
}

void SceneManager::DrawRTV()
{
	scene_->DrawRTV();
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	// 次シーンを生成
	nextScene_ = sceneFactory_->CreateScene(sceneName);
}

