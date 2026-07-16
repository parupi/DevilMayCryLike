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
	// 進行度を進める（引数は発生したイベントの種類。現在表示中のチュートリアルと一致する場合のみ進行する）
	virtual void StepTutorial(TutorialState state) = 0;
};