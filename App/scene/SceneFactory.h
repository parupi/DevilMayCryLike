#pragma once

#include "Scene/AbstractSceneFactory.h"

// このゲーム用のシーン工場
class SceneFactory : public AbstractSceneFactory
{
public:
	// 新しいシーンを生成
	std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override;

};

