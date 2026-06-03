#include "SceneManager.h"
#include <cassert>
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Object/Object3dManager.h>

std::unique_ptr<SceneManager> SceneManager::instance;
std::once_flag SceneManager::initInstanceFlag;

SceneManager* SceneManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance.reset(new SceneManager());
		});
	return instance.get();
}

void SceneManager::Finalize()
{
	// 最後のシーンの終了と解放
	scene_->Finalize();
	delete scene_;
	scene_ = nullptr;

	instance.reset();
}

void SceneManager::Update()
{
	// 次シーンの予約があるなら
	if (nextScene_) {
		// 旧シーンの終了
		if (scene_) {
			scene_->Finalize();
			delete scene_;

			// 全オブジェクトを削除
			RendererManager::GetInstance()->RemoveDeadObjects();
			CollisionManager::GetInstance()->RemoveDeadObjects();
			Object3dManager::GetInstance()->RemoveDeadObject();
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

#ifdef _DEBUG
void SceneManager::DebugUpdate()
{
	scene_->DebugUpdate();
}
#endif


