#include "SceneFactory.h"
#include <TitleScene.h>
#include <GameScene.h>
//#include <Result.h>
//#include <TutorialScene.h>

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
    else if (sceneName == "RESULT") {
        //newScene = new Result();
    }
    else if (sceneName == "TUTORIAL") {
        //newScene = new TutorialScene();
    }

    return newScene;
}