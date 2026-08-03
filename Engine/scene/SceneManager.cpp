#include "SceneManager.h"
#include <cassert>
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Object/Object3dManager.h>
#include <Graphics/Device/DirectXManager.h>


SceneManager& SceneManager::GetInstance() {
	static SceneManager instance;
	return instance;
}

void SceneManager::Finalize() {
	// 最後のシーンの終了と解放
	scene_->Finalize();
	scene_.reset();
}

void SceneManager::Update() {
	// 次シーンの予約があるなら
	if (nextScene_) {
		// 旧シーンの終了
		if (scene_) {
			scene_->Finalize();
			scene_.reset();

			// 全オブジェクトを削除
			RendererManager::GetInstance().RemoveDeadObjects();
			CollisionManager::GetInstance().RemoveDeadObjects();
			Object3dManager::GetInstance().RemoveDeadObject();
		}

		// シーンの切り替え
		scene_ = std::move(nextScene_);
		currentSceneName_ = nextSceneName_;

		scene_->SetSceneManager(this);

		// 次シーンを初期化する。
		// ここは1フレームで大量のテクスチャを読むので、アップロード用のステージングが
		// まるごと積み上がらないようスコープで囲む（一定量ごとに解放される）。
		// Draw より前に呼ばれるので、スコープ内のコマンドリスト確定は安全。
		{
			DirectXManager::UploadScope uploadScope(dxManager_);
			scene_->Initialize();
		}
	}

	scene_->Update();
}

void SceneManager::Draw() {
	scene_->Draw();
}

void SceneManager::ChangeScene(const std::string& sceneName) {
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	// 次シーンを生成
	nextScene_ = sceneFactory_->CreateScene(sceneName);
	nextSceneName_ = sceneName;
}

#ifdef _DEBUG
void SceneManager::DebugUpdate() {
	scene_->DebugUpdate();
}

void SceneManager::ReloadCurrentScene() {
	if (currentSceneName_.empty() || IsSceneChangePending()) {
		return;
	}
	ChangeScene(currentSceneName_);
}
#endif

