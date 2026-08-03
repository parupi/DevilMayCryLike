#pragma once

#include "BaseScene.h"
#include <string>
#include <memory>
#include <vector>

/// <summary>
/// シーン工場(概念)
/// </summary>
class AbstractSceneFactory
{
public:
	// 仮想デストラクタ
	virtual ~AbstractSceneFactory() = default;
	// シーン生成
	virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;

	/// <summary>
	/// CreateScene() に渡せる名前の一覧。エディタのSceneメニューが使う。
	/// 実装しなければ空（メニューには何も並ばない）。
	/// </summary>
	virtual std::vector<std::string> GetSceneNames() const { return {}; }
};

