#include "SceneFactory.h"
#include "Scene/BaseScene.h"
#include "Scene/TitleScene.h"
#include "Scene/GameScene/GameScene.h"
#include "Scene/ClearScene.h"
#include "Scene/SampleScene.h"
#include "Scene/EditScene.h"

std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName)
{
    if (sceneName == "TITLE")    return std::make_unique<TitleScene>();
    if (sceneName == "GAMEPLAY") return std::make_unique<GameScene>();
    if (sceneName == "CLEAR")    return std::make_unique<ClearScene>();
    if (sceneName == "Edit")     return std::make_unique<EditScene>();
    if (sceneName == "SAMPLE")   return std::make_unique<SampleScene>();
    return nullptr;
}