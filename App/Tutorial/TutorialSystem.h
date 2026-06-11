#pragma once
#include <unordered_map>
#include <memory>
#include "Tutorial.h"

enum class TutorialState {
	Move, // 移動のチュートリアル
	AttackA, // 攻撃Aコンボのチュートリアル
	AttackB, // 攻撃Bコンボのチュートリアル
	LockOn, // ロックオンのチュートリアル
	RoundUpAttack, // 切り上げ攻撃のチュートリアル

	Count, // チュートリアルの種類の数
};

class TutorialSystem {
public:
	TutorialSystem() = default;
	~TutorialSystem() = default;
	// 初期化
	void Initialize();
	// 更新
	void Update();
	// チュートリアルの開始
	void StartTutorial(TutorialState state);
	// 次のチュートリアルに進む
	//void NextTutorial();
	// チュートリアルの終了
	void EndTutorial();
private:
	// チュートリアルを切り替える為の更新関数
	//void ChangeTutorialUpdate();
	TutorialState state_ = TutorialState::Move; // 現在のチュートリアルの状態

	// チュートリアルのマップ
	std::unordered_map<TutorialState, std::unique_ptr<Tutorial>> tutorials_;
	// 現在のチュートリアル
	Tutorial* currentTutorial_ = nullptr;
	// 切り替え用のフラグ
	bool isTutorialChanging_ = false;
};

