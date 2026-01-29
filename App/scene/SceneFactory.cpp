#include "SceneFactory.h"
#include "scene/BaseScene.h"
#include "scene/TitleScene.h"
#include "scene/GameScene/GameScene.h"
#include "scene/ClearScene.h"
#include "scene/SampleScene.h"
#include "scene/EditScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName)
{
    // 次のシーンを生成
    BaseScene* newScene = nullptr;

    if (sceneName == "TITLE") {
        newScene = new TitleScene();
    }
    else if (sceneName == "GAMEPLAY") {
        newScene = new GameScene();
    }
    else if (sceneName == "CLEAR") {
        newScene = new ClearScene();
    }
    else if (sceneName == "Edit") {
        newScene = new EditScene();
    } else if (sceneName == "SAMPLE") {
        newScene = new SampleScene();
    }

    return newScene;
}