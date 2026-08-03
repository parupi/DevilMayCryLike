#pragma once
#include <Scene/BaseScene.h>
#include <Scene/AbstractSceneFactory.h>
#include <memory>
#include <mutex>

class DirectXManager;

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
	// シーンの変更
	void ChangeScene(const std::string& sceneName);

	// 実行中のシーン名（ChangeScene で指定された名前）
	const std::string& GetCurrentSceneName() const { return currentSceneName_; }
	// 次のシーンが予約済みか。ChangeScene は二重予約でassertするので、外から先に確認する用
	bool IsSceneChangePending() const { return nextScene_ != nullptr; }
	// シーンファクトリー（エディタがシーン名の一覧を引く）
	AbstractSceneFactory* GetSceneFactory() const { return sceneFactory_; }

#ifdef _DEBUG
	void DebugUpdate();
	// 実行中のシーンを作り直す
	void ReloadCurrentScene();
#endif

private:
	// 実行中のシーン
	std::unique_ptr<BaseScene> scene_;
	// 次のシーン
	std::unique_ptr<BaseScene> nextScene_;
	// ChangeScene に渡された名前。実際に切り替わった時点で currentSceneName_ に移る
	std::string currentSceneName_;
	std::string nextSceneName_;
	// シーンファクトリー
	AbstractSceneFactory* sceneFactory_ = nullptr;
	// シーン初期化を DirectXManager::UploadScope で囲むためだけに保持する。
	// 未設定(nullptr)でも動作は変わらない（ステージングの解放が EndDraw まで遅れるだけ）
	DirectXManager* dxManager_ = nullptr;

public:
	// シーンファクトリーのsetter
	void SetSceneFactory(AbstractSceneFactory* sceneFactory) { sceneFactory_ = sceneFactory; }
	// 最初のシーンが初期化される前（＝最初の Update より前）に設定すること
	void SetDXManager(DirectXManager* dxManager) { dxManager_ = dxManager; }

};