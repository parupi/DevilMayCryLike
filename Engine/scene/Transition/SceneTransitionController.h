#pragma once
#include <string>
#include <mutex>
#include <memory>
class SceneTransitionController
{
private:
	SceneTransitionController() = default;
	SceneTransitionController(const SceneTransitionController&) = delete;
	SceneTransitionController& operator=(const SceneTransitionController&) = delete;
public:
	// インスタンスの取得
	static SceneTransitionController& GetInstance();
	// シーンの切り替えをリクエスト
	void RequestSceneChange(const std::string& nextScene, bool useTransition = true);
	/// <summary>
	/// シーン切り替えを伴わないフェードインだけを始める。
	///
	/// 起動直後の最初のシーンは RequestSceneChange を通らないので、暗転からの明けが無く
	/// いきなり画面が出てしまう。そのシーンの Initialize から呼ぶと同じ入り方に揃う。
	/// 切り替えの最中(Idle以外)なら何もしないので、通常のシーン遷移と二重には走らない。
	/// </summary>
	void BeginFadeIn();
	/// <summary>
	/// アプリの終了を要求する。タイトルの QUIT から呼ぶ。
	///
	/// 暗転しきってからウィンドウを閉じるので、シーン切り替えと同じ見え方になる。
	/// 暗転が終わったあとはメインループが自然に抜けて、通常の終了処理を通る
	/// </summary>
	void RequestApplicationQuit();
	// 更新処理
	void Update();
	// 終了処理
	void Finalize();

private:
	enum class State {
		Idle,
		FadeOut, // フェードアウト中
		ChangingScene, // シーンの切り替え
		FadeIn, // フェードイン中
		Quitting, // 暗転しきったらアプリを終了する
	}state_ = State::Idle;

	std::string nextScene_;
	bool useTransition_ = false;
};

