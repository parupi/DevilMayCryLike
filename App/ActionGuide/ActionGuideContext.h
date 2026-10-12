#pragma once
#include <string>

class Player;
class InputContext;

/// <summary>
/// 1フレームぶんの判定に使う状態のスナップショット（仕様書 §5.1）。
/// ポインタは持たない。中身は ActionGuideContextBuilder が既存のシステムから集める
/// </summary>
struct ActionGuideContext {
	/// <summary>プレイヤーのステート名（PlayerStateBase::GetDebugName。Idle / Move / Dodge など）</summary>
	std::string state;
	/// <summary>攻撃中の攻撃名。攻撃中でなければ空</summary>
	std::string currentAttack;

	bool isAttacking = false;
	bool isCharging = false;
	bool isGrounded = false;
	bool isLockOn = false;
	bool counterWindowOpen = false;

	bool isStunned = false;     ///< ノックバック中
	bool isDead = false;        ///< 死亡演出中
	bool isInCutscene = false;  ///< ゲームのカメラ以外が映っている（ボス出現・クリアなど）
	bool isInputLocked = false; ///< プレイヤーの入力を受け付けていない（開始演出・ポーズ・クリア）
	bool isInBattle = false;    ///< 戦闘中（スタイルスコアの戦闘フラグ）
};

/// <summary>既存のシステムから ActionGuideContext を作る（仕様書 §5.1 / §6-1）</summary>
class ActionGuideContextBuilder
{
public:
	static ActionGuideContext Build(Player& player, const InputContext* inputContext);
};
