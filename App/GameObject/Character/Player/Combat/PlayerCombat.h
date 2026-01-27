#pragma once
#include <unordered_map>
#include <GameObject/Character/Player/State/Attack/PlayerStateAttack.h>
#include "AttackPlayer.h"
#include <memory>
#include <string>
#include <imgui.h>

enum class AttackType {
	Normal,
	RoundUp,
	LungeThrust,
	Air,
};

enum class InputButton {
	X,
	Y,
	None
};

enum class StickDirection {
	None,
	ToEnemy,
	AwayFromEnemy,
	Any
};

struct AttackInputCondition
{
	InputButton button;          // X / Y
	bool requireLockOn;    // ロックオン必須か
	StickDirection stick;            // 敵基準のスティック方向
};

struct AttackNode
{
	std::string name;
	// 派生先（攻撃名で管理）
	std::vector<std::string> nextAttacks;
};

class PlayerCombat
{
public:
	PlayerCombat() = default;
	~PlayerCombat() = default;
	// 初期化
	void Initialize(Player* player);
	// 更新
	void Update();
	// 描画
	void Draw();
	// 外部から攻撃をリクエストする
	void RequestAttack(AttackType type);
	// 攻撃を変更する TODO : FSMじゃなくてStateStackを使う
	void ChangeState(const std::string& stateName);

	std::string GetAttackStateNameByIndex(int32_t index) const;

	int32_t GetAttackStateCount() const;

	bool IsAttacking() const { return !currentState_.empty(); }

	const AttackNode& GetAttackNode(const std::string& name) const{ return attackGraph_.at(name); }
private:
	// Jsonの名前からステートを生成
	void CreateState();

	void DrawAttackDataEditorUI();
	// 攻撃を追加
	void AddAttackState(const std::string& attackName);

	void DrawAttackDataEditor(PlayerStateAttack* attack);

	AttackNode LoadAttackNode(const std::string& attackName);

	void DrawAttackNodeEditor(AttackNode& node);

	void DrawAttackDerivativeEditorUI();

private:
	std::unordered_map<std::string, AttackNode> attackGraph_;
	// ステート名とステートインスタンスのマップ
	std::unordered_map<std::string, std::unique_ptr<PlayerStateAttack>> states_;
	// 現在のステート
	std::vector<PlayerStateAttack*> currentState_;

	GlobalVariables* global_ = GlobalVariables::GetInstance();
	// プレイヤーのポインターを取得 TODO : なんか良い感じの受け渡し方法にしたい
	Player* player_ = nullptr;

	std::unique_ptr<AttackPlayer> attackPlayer_ = nullptr;
};

