#include "SceneTransitionController.h"
#include "Scene/Transition/TransitionManager.h"
#include <Scene/SceneManager.h>
#include <Graphics/Rendering/Sprite/SpriteManager.h>

std::unique_ptr<SceneTransitionController> SceneTransitionController::instance;
std::once_flag SceneTransitionController::initInstanceFlag;

SceneTransitionController* SceneTransitionController::GetInstance()
{
    std::call_once(initInstanceFlag, []() {
        instance.reset(new SceneTransitionController());
        });
    return instance.get();
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
    instance.reset();
}
