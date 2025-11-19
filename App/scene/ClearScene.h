#pragma once
#include "scene/BaseScene.h"
#include <2d/Sprite.h>
#include <vector>
#include <memory>
#include <GameObject/UI/ClearUI/ClearUI.h>
class ClearScene : public BaseScene
{
public:
	ClearScene() = default;
	~ClearScene() = default;

	// 初期化
	void Initialize() override;
	// 終了
	void Finalize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;
	void DrawRTV() override;

#ifdef _DEBUG
	void DebugUpdate() override;
#endif // _DEBUG

private:
	std::unique_ptr<ClearUI> clearUI_;

};

