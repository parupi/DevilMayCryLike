#include "SceneFactory.h"
#include "Scene/BaseScene.h"
#include "Scene/TitleScene.h"
#include "Scene/GameScene/GameScene.h"
#include "Scene/ClearScene.h"
#include "Scene/SampleScene.h"
#include "Scene/EditScene.h"

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
    } 
    else if (sceneName == "SAMPLE") {
        newScene = new SampleScene();
    }

    return newScene;
}