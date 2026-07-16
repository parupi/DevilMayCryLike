#pragma once
#include "GameObject/Character/Enemy/GruntMelee/GruntMelee.h"

/// <summary>
/// チュートリアル用の敵。
/// 挙動は GruntMelee（移動・攻撃する近接敵）と同じだが、
/// 全チュートリアルが完了するまでは HP が 0 になっても死亡しない。
/// チュートリアル完了後は通常の敵と同じく倒せるようになる。
/// レベルエディタから "TutorialDummy" として配置できる。
/// </summary>
class TutorialDummy : public GruntMelee
{
public:
	TutorialDummy(std::string objectName) : GruntMelee(objectName) {}

protected:
	/// <summary>
	/// 全チュートリアルが完了していれば true（＝倒せる）。
	/// 完了していない間は false を返して死亡を抑制する。
	/// </summary>
	bool CanDie() const override;
};
