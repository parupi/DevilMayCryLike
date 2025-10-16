#include "SceneTransitionController.h"
#include "scene/Transition/TransitionManager.h"
#include <scene/SceneManager.h>
#include <2d/SpriteManager.h>

SceneTransitionController* SceneTransitionController::instance = nullptr;
std::once_flag SceneTransitionController::initInstanceFlag;

SceneTransitionController* SceneTransitionController::GetInstance()
{
    std::call_once(initInstanceFlag, []() {
        instance = new SceneTransitionController();
        });
    return instance;
}

void SceneTransitionController::RequestSceneChange(const std::string& nextScene, bool useTransition)
{
    if (state_ != State::Idle) return;

    nextScene_ = nextScene;
    useTransition_ = useTransition;

    if (useTransition_) {
        TransitionManager::GetInstance()->Play(true);
        state_ = State::FadeOut;
    } else {
        SceneManager::GetInstance()->ChangeScene(nextScene_);
        state_ = State::FadeIn;
    }
}

void SceneTransitionController::Update()
{
    auto* tm = TransitionManager::GetInstance();

    switch (state_) {
    case State::FadeOut:
        tm->Update();
        if (tm->IsFinished()) {
            SceneManager::GetInstance()->ChangeScene(nextScene_);
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

void SceneTransitionController::Draw()
{
    SpriteManager::GetInstance()->DrawSet(BlendMode::kNormal);
    TransitionManager::GetInstance()->Draw();
}

void SceneTransitionController::Finalize()
{
    delete instance;
    instance = nullptr;
}
