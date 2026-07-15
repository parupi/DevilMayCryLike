#include "SceneTransitionController.h"
#include "Scene/Transition/TransitionManager.h"
#include <Scene/SceneManager.h>


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
    default:
        break;
    }

}

void SceneTransitionController::Finalize()
{
}