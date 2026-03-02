#pragma once
#include "scene/BaseScene.h"
#include "input/InputContext.h"
class EditScene : public BaseScene
{
public:
	EditScene() = default;
	~EditScene() = default;

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
	// 入力をまとめたクラス
	std::unique_ptr<InputContext> inputContext_ = nullptr;
};

