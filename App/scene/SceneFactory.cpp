#include "SceneFactory.h"
#include "scene/BaseScene.h"
#include <scene/TitleScene.h>
#include <scene/GameScene.h>
//#include <Result.h>
//#include <TutorialScene.h>
#include "scene/SampleScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName)
{
    // 次のシーンを生成
    BaseScene* newScene = nullptr;

    if (sceneName == "TITLE") {
        //newScene = new TitleScene();
    }
    else if (sceneName == "GAMEPLAY") {
        newScene = new GameScene();
    }
    else if (sceneName == "RESULT") {
        //newScene = new Result();
    }
    else if (sceneName == "TUTORIAL") {
        //newScene = new TutorialScene();
    } else if (sceneName == "SAMPLE") {
        newScene = new SampleScene();
    }

    return newScene;
}