#pragma once
#include <unordered_map>
#include <GameObject/Character/Player/State/Attack/PlayerStateAttack.h>
#include "GameObject/Character/Player/Controller/PlayerInput.h"
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

enum class StickDirection {
	None,
	ToEnemy,
	AwayFromEnemy,
	Any
};

struct AttackInputCondition
{
	InputButton button = InputButton::None;
	bool requireLockOn = false;
	StickDirection stick = StickDirection::None;
};

struct AttackNode
{
	std::string name;
	bool isRootAttack = false;
	bool isAir = false;
	AttackInputCondition condition;
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
	void Update(float deltaTime);
	// 描画
	void Draw();
	// 攻撃を追加する
	void AddState(const std::string& stateName);
	// 現在攻撃中かどうか
	bool IsAttacking() const { return !currentState_.empty(); }
	// 攻撃を強制中断する（被弾時など）
	void InterruptCombat();
	// 攻撃ノードを取得
	const AttackNode& GetAttackNode(const std::string& name) const{ return attackGraph_.at(name); }
	// プレイヤーからのコマンドを受け取って処理する
	void ExecuteCommand(const PlayerCommand& command);
private:
	// Jsonの名前からステートを生成
	void CreateState();
	// 攻撃データエディタのUIを描画
	void DrawAttackDataEditorUI();
	// 攻撃を追加
	void AddAttackState(const std::string& attackName);
	// 攻撃データエディタのUIを描画
	void DrawAttackDataEditor(PlayerStateAttack* attack);

	AttackNode LoadAttackNode(const std::string& attackName);

	void DrawAttackNodeEditor(const std::string& attackName, AttackNode& node);

	void DrawAttackDerivativeEditorUI();

private:
	std::unordered_map<std::string, AttackNode> attackGraph_;
	// ステート名とステートインスタンスのマップ
	std::unordered_map<std::string, std::unique_ptr<PlayerStateAttack>> states_;
	// 現在のステート
	std::vector<PlayerStateAttack*> currentState_;

	GlobalVariables* global_ = &GlobalVariables::GetInstance();
	// プレイヤーの参照を保持
	Player* player_ = nullptr;
	// 攻撃再生クラス
	std::unique_ptr<AttackPlayer> attackPlayer_ = nullptr;
	// コンボのリセットタイマー
	float comboResetTimer_ = 0.0f;
	// 次の攻撃入力を待っているかどうか
	bool waitingForNextCombo_ = false;
};