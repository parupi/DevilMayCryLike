#pragma once
#include <BaseScene.h>
#include <AbstractSceneFactory.h>
#include <memory>
#include <mutex>
class SceneManager
{
private:
	static std::unique_ptr<SceneManager> instance;
	static std::once_flag initInstanceFlag;

	SceneManager(SceneManager&) = default;
	SceneManager& operator=(SceneManager&) = default;
public:
	SceneManager() = default;
	~SceneManager() = default;
	// シングルトンインスタンスの取得
	static SceneManager* GetInstance();
	// 次のシーンを予約する
	void SetNextScene(BaseScene* nextScene) { nextScene_ = nextScene; }
	// 終了
	void Finalize();
	// 更新
	void Update();
	// 描画
	void Draw();
	// シーンの変更
	void ChangeScene(const std::string& sceneName);

private:
	// 実行中のシーン
	BaseScene* scene_ = nullptr;
	// 次のシーン
	BaseScene* nextScene_ = nullptr;
	// シーンファクトリー
	AbstractSceneFactory* sceneFactory_ = nullptr;

public:
	// シーンファクトリーのsetter
	void SetSceneFactory(AbstractSceneFactory* sceneFactory) { sceneFactory_ = sceneFactory; }

};