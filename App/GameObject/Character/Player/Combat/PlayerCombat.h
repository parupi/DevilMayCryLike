#pragma once
#include <unordered_map>
#include <GameObject/Character/Player/State/Attack/PlayerStateAttack.h>
#include <memory>
#include <string>
#include <imgui.h>
//#include <imgui_node_editor.h>
//#include <imgui_internal.h>

enum class AttackType {
	Normal,
	RoundUp,
	LungeThrust,
};

struct AttackNode
{
	std::string name;
	// 派生先（攻撃名で管理）
	std::vector<std::string> nextAttacks;
};

using NodeID = uint32_t;
using PinID = uint32_t;

struct AttackNodeEditorData
{
	NodeID nodeId;
	PinID  inputPin;
	std::vector<PinID> outputPins;

	ImVec2 position;
};

//namespace ed = ax::NodeEditor;

class PlayerCombat
{
public:
	PlayerCombat() = default;
	~PlayerCombat();
	// 初期化
	void Initialize(Player* player);

	//void InitializeNodeEditor();

	//void FinalizeNodeEditor();
	// 更新
	void Update();
	// 描画前のセットアップ
	void DrawAttackGraphEditor();
	// 外部から攻撃をリクエストする
	void RequestAttack(AttackType type);
	// 攻撃を変更する TODO : FSMじゃなくてStateStackを使う
	void ChangeState(const std::string& stateName);

	std::string GetAttackStateNameByIndex(int32_t index) const;

	int32_t GetAttackStateCount() const;

	bool IsAttacking() const { return !currentState_.empty(); }

	std::unordered_map<std::string, AttackNode> attackGraph_;
	std::unordered_map<std::string, AttackNodeEditorData> editorData_;

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
	//// ノード生成
	//void CreateEditorNode(const std::string& attackName);
	//// ノードの表示
	//void DrawAttackNode(const std::string& name);

private:
	//ed::EditorContext* editorContext_ = nullptr;
	// ステート名とステートインスタンスのマップ
	std::unordered_map<std::string, std::unique_ptr<PlayerStateAttack>> states_;
	// 現在のステート
	std::vector<PlayerStateAttack*> currentState_;

	GlobalVariables* global_ = GlobalVariables::GetInstance();
	// プレイヤーのポインターを取得 TODO : なんか良い感じの受け渡し方法にしたい
	Player* player_ = nullptr;
};

