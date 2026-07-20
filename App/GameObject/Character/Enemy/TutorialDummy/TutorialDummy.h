#pragma once
#include "GameObject/Character/Enemy/GruntMelee/GruntMelee.h"

/// <summary>
/// チュートリアル用の敵（練習台）。
/// 移動は GruntMelee と同じだが、攻撃行動は一切行わない。
/// また、全チュートリアルが完了するまでは HP が 0 になっても死亡しない。
/// チュートリアル完了後は通常の敵と同じく倒せるようになる。
/// レベルエディタから "TutorialDummy" として配置できる。
/// </summary>
class TutorialDummy : public GruntMelee
{
public:
	TutorialDummy(std::string objectName) : GruntMelee(objectName) {}

	/// <summary>
	/// 練習台なので攻撃行動は行わない。
	/// CombatIdle が攻撃を選ぼうとしたときに移動行動へ置き換えられる。
	/// </summary>
	bool CanAttack() const override { return false; }

protected:
	/// <summary>
	/// 全チュートリアルが完了していれば true（＝倒せる）。
	/// 完了していない間は false を返して死亡を抑制する。
	/// </summary>
	bool CanDie() const override;
};
