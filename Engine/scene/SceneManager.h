#pragma once
#include <Scene/BaseScene.h>
#include <Scene/AbstractSceneFactory.h>
#include <memory>
#include <mutex>
class SceneManager
{
private:
	SceneManager() = default;
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
public:

	// シングルトンインスタンスの取得
	static SceneManager& GetInstance();
	// 次のシーンを予約する
	void SetNextScene(std::unique_ptr<BaseScene> nextScene) { nextScene_ = std::move(nextScene); }
	// 終了
	void Finalize();
	// 更新
	void Update();
	// 描画
	void Draw();
	// RTVに描画する
	void DrawRTV();
	// シーンの変更
	void ChangeScene(const std::string& sceneName);

#ifdef _DEBUG
	void DebugUpdate();
#endif

private:
	// 実行中のシーン
	std::unique_ptr<BaseScene> scene_;
	// 次のシーン
	std::unique_ptr<BaseScene> nextScene_;
	// シーンファクトリー
	AbstractSceneFactory* sceneFactory_ = nullptr;

public:
	// シーンファクトリーのsetter
	void SetSceneFactory(AbstractSceneFactory* sceneFactory) { sceneFactory_ = sceneFactory; }

};