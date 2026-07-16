#pragma once
#include <unordered_map>
#include <memory>
#include "../Tutorial.h"
#include "../Service/TutorialService.h"
#include "Tutorial/Decoration/TutorialDecoration.h"

class TutorialSystem : public TutorialService {
public:
	TutorialSystem() = default;
	~TutorialSystem() = default;
	// 初期化
	void Initialize();
	// 更新
	void Update();
	// チュートリアルの開始
	void StartTutorial(TutorialState state) override;
	// チュートリアルの終了
	void EndTutorial();
	// 進行度を進める
	void StepTutorial(TutorialState state) override;
	// 全チュートリアルが完了したか
	bool IsAllFinished() const override { return isAllFinished_; }
private:
	// 現在のチュートリアルを終了し、次のチュートリアルへ自動的に進める
	void AdvanceTutorial();

	TutorialState state_ = TutorialState::Move; // 現在のチュートリアルの状態

	// チュートリアルのマップ
	std::unordered_map<TutorialState, std::unique_ptr<Tutorial>> tutorials_;
	// 現在のチュートリアル
	Tutorial* currentTutorial_ = nullptr;
	// フェードアウト中の直前のチュートリアル（消えきるまで更新を続ける）
	Tutorial* previousTutorial_ = nullptr;
	// 切り替え用のフラグ
	bool isTutorialChanging_ = false;
	// 全チュートリアルを完了したか（最後の種類まで進み切ったら true）
	bool isAllFinished_ = false;
	// 装飾表示用のクラス
	std::unique_ptr<TutorialDecoration> decoration_ = nullptr;
};

