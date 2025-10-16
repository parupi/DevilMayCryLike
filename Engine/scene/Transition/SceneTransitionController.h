#pragma once
#include <string>
#include <mutex>
class SceneTransitionController
{
private:
	static SceneTransitionController* instance;
	static std::once_flag initInstanceFlag;

	SceneTransitionController() = default;
	~SceneTransitionController() = default;
	SceneTransitionController(SceneTransitionController&) = default;
	SceneTransitionController& operator=(SceneTransitionController&) = default;
public:
	// インスタンスの取得
	static SceneTransitionController* GetInstance();
	// シーンの切り替えをリクエスト
	void RequestSceneChange(const std::string& nextScene, bool useTransition = true);
	// 更新処理
	void Update();
	// 描画処理
	void Draw();
	// 終了処理
	void Finalize();

private:
	enum class State {
		Idle,
		FadeOut, // フェードアウト中
		ChangingScene, // シーンの切り替え
		FadeIn, // フェードイン中
	}state_ = State::Idle;

	std::string nextScene_;
	bool useTransition_ = false;
};

