#pragma once

class Sprite;
// 背景マスクやディバイダ―を表示するクラス
class TutorialDecoration {
public:
	TutorialDecoration() = default;
	~TutorialDecoration() = default;
	// 初期化
	void Initialize();
	// 更新処理
	void Update();
	// 表示開始

	// 表示終了



private:
	Sprite* musk_ = nullptr;
	Sprite* upDivider_ = nullptr;
	Sprite* underDivider_ = nullptr;
};