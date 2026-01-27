#pragma once
#include <scene/BaseScene.h>
#include <scene/AbstractSceneFactory.h>
#include <memory>
#include <mutex>
class SceneManager
{
private:
	static SceneManager* instance;
	static std::once_flag initInstanceFlag;

	SceneManager() = default;
	~SceneManager() = default;
	SceneManager(SceneManager&) = default;
	SceneManager& operator=(SceneManager&) = default;
public:

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
	// RTVに描画する
	void DrawRTV();
	// シーンの変更
	void ChangeScene(const std::string& sceneName);

#ifdef _DEBUG
	void DebugUpdate();
#endif

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