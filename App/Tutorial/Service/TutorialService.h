#pragma once

enum class TutorialState {
	Move, // 移動のチュートリアル
	AttackA, // 攻撃Aコンボのチュートリアル
	AttackB, // 攻撃Bコンボのチュートリアル
	LockOn, // ロックオンのチュートリアル
	RoundUpAttack, // 切り上げ攻撃のチュートリアル

	Count, // チュートリアルの種類の数
};

// プレイヤーに渡すための基底クラス
class TutorialService {
public:
	// チュートリアルの開始
	virtual void StartTutorial(TutorialState state) = 0;
	// 進行度を進める
	virtual void StepTutorial() = 0;
};