#include "SceneTransitionController.h"
#include "Scene/Transition/TransitionManager.h"
#include <Scene/SceneManager.h>
#include <Platform/WindowManager.h>


SceneTransitionController& SceneTransitionController::GetInstance()
{
	static SceneTransitionController instance;
	return instance;
}

void SceneTransitionController::RequestSceneChange(const std::string& nextScene, bool useTransition)
{
    if (state_ != State::Idle) return;

    nextScene_ = nextScene;
    useTransition_ = useTransition;

    if (useTransition_) {
        TransitionManager::GetInstance().Play(true);
        state_ = State::FadeOut;
    } else {
        SceneManager::GetInstance().ChangeScene(nextScene_);
        state_ = State::FadeIn;
    }
}

void SceneTransitionController::BeginFadeIn()
{
    // 通常のシーン遷移の途中なら、そちらのフェードインに任せる
    if (state_ != State::Idle) return;

    TransitionManager::GetInstance().Play(false);
    state_ = State::FadeIn;
}

void SceneTransitionController::RequestApplicationQuit()
{
    if (state_ != State::Idle) return;

    TransitionManager::GetInstance().Play(true);
    state_ = State::Quitting;
}

void SceneTransitionController::Update()
{
    TransitionManager* tm = &TransitionManager::GetInstance();

    switch (state_) {
    case State::FadeOut:
        tm->Update();
        if (tm->IsFinished()) {
            SceneManager::GetInstance().ChangeScene(nextScene_);
            tm->Play(false);
            state_ = State::FadeIn;
        }
        break;
    case State::FadeIn:
        tm->Update();
        if (tm->IsFinished()) {
            state_ = State::Idle;
        }
        break;
    case State::Quitting:
        tm->Update();
        if (tm->IsFinished()) {
            // 暗転したまま、次の ProcessMessage でメインループを抜ける。
            // Idle へ戻さないのは、この後に別の遷移を走らせないため
            WindowManager::RequestQuit();
        }
        break;
    default:
        break;
    }

}

void SceneTransitionController::Finalize()
{
}