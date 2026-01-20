#include "PlayerCombat.h"
#include "GameObject/Character/Player/Player.h"


//namespace ed = ax::NodeEditor;

PlayerCombat::~PlayerCombat()
{
	//FinalizeNodeEditor();
}

void PlayerCombat::Initialize(Player* player)
{
	player_ = player;

	CreateState();
	//InitializeNodeEditor();
}

//void PlayerCombat::InitializeNodeEditor()
//{
//	//ed::Config config;
//	//config.SettingsFile = "AttackGraphEditor.json"; // 自動保存用（任意）
//
//	//editorContext_ = ed::CreateEditor(&config);
//}
//
//void PlayerCombat::FinalizeNodeEditor()
//{
//	//if (editorContext_) {
//	//	ed::DestroyEditor(editorContext_);
//	//	editorContext_ = nullptr;
//	//}
//}

void PlayerCombat::Update()
{
	//static ed::NodeId nodeId = 1;
	//static ed::PinId  inPin = 2;
	//static ed::PinId  outPin = 3;
	//ed::Begin("TestEditor");
	//ed::BeginNode(nodeId);
	//ImGui::Text("Test Node");

	//ed::BeginPin(inPin, ed::PinKind::Input);
	//ImGui::Text("In");
	//ed::EndPin();

	//ed::BeginPin(outPin, ed::PinKind::Output);
	//ImGui::Text("Out");
	//ed::EndPin();

	//ed::EndNode();
	//ed::End();

	DrawAttackDataEditorUI();
	for (auto& state : states_) {
		state.second->UpdateAttackData();
	}
	
	if (currentState_.empty()) return;

	auto& top = currentState_.back();

	auto req = top->Update(*player_);

	if (top->IsFinished()) {
		top->Exit(*player_);
		currentState_.pop_back();
	}

	switch (req.type) {
	case AttackRequest::Jump:
		player_->ChangeState("Jump");
		break;
	case AttackRequest::Air:
		player_->ChangeState("Air");
		break;
	case AttackRequest::ChangeAttack:
		ChangeState(req.nextAttack);
		break;
	}
}

void PlayerCombat::DrawAttackGraphEditor()
{
	//ed::SetCurrentEditor(editorContext_);

	//ed::Begin("Attack Graph");
	//// ノード描画
	//ed::End();

	//ed::SetCurrentEditor(nullptr);
}

void PlayerCombat::RequestAttack(AttackType type)
{
	switch (type) {
	case AttackType::Normal:
		ChangeState("AttackComboA1");
		break;
	case AttackType::RoundUp:
		ChangeState("AttackHighTime");
		break;
	case AttackType::LungeThrust:

		break;
	}
}

void PlayerCombat::ChangeState(const std::string& stateName)
{
	if (currentState_.empty()) {
		auto it = states_[stateName].get();
		it->Enter(*player_);
		currentState_.push_back(it);
		return;
	}

	if (currentState_.back()->CanBeInterrupted()) {
		auto it = states_[stateName].get();
		it->Enter(*player_);
		currentState_.push_back(it);
		return;
	}
}

std::string PlayerCombat::GetAttackStateNameByIndex(int32_t index) const
{
	int i = 0;
	for (const auto& [name, state] : states_) {
		if (i == index) return name;
		++i;
	}
	return "";
}

int32_t PlayerCombat::GetAttackStateCount() const
{
	int32_t count = 0;
	for (const auto& [_, state] : states_) {
		++count;
	}
	return count;
}

void PlayerCombat::CreateState()
{
	auto list = global_->GetGroupNames("PlayerAttack");
	for (std::string name : list) {
		global_->LoadFile("PlayerAttack", name);
		states_[name] = std::make_unique<PlayerStateAttack>(name);
		states_[name]->UpdateAttackData();
		attackGraph_[name] = LoadAttackNode(name);
	}
}

void PlayerCombat::DrawAttackDataEditorUI()
{
#ifdef _DEBUG
	// 攻撃ステートを収集
	std::vector<PlayerStateAttack*> attackStates;
	std::vector<std::string> attackNames;
	for (auto& [name, attack] : states_) {
		attackStates.push_back(attack.get());
		attackNames.push_back(attack->name_);
	}

	// 攻撃ステートが存在しない場合は処理しない
	if (attackStates.empty()) return;

	// コンボによる選択 UI
	static int currentIndex = 0;
	if (currentIndex >= attackStates.size()) currentIndex = 0;

	std::vector<const char*> nameCStrs;
	for (const auto& name : attackNames) {
		nameCStrs.push_back(name.c_str());
	}

	static char newAttackName[64] = "";

	ImGui::Begin("Attack Editor");

	//-----------------------------
	// 攻撃追加UI
	//-----------------------------
	ImGui::InputText("New Attack Name", newAttackName, IM_ARRAYSIZE(newAttackName));

	ImGui::SameLine();
	if (ImGui::Button("Add Attack")) {
		if (strlen(newAttackName) > 0) {
			AddAttackState(newAttackName);
			newAttackName[0] = '\0'; // 入力欄クリア
		}
	}

	ImGui::Separator();

	if (ImGui::Combo("Select Attack", &currentIndex, nameCStrs.data(), static_cast<int>(nameCStrs.size()))) {
		// 選択が変わったら必要に応じて処理
	}

	// 選択中の攻撃ステートのエディタを表示
	PlayerStateAttack* selectedAttack = attackStates[currentIndex];
	if (selectedAttack) {
		DrawAttackDataEditor(selectedAttack);
	}

	ImGui::End();
#endif // IMGUI
}

void PlayerCombat::AddAttackState(const std::string& attackName)
{
	// すでに存在しているなら追加しない
	if (states_.contains(attackName)) {
		return;
	}

	// データファイル（Group）を作成
	global_->CreateGroup(attackName);

	// ステート生成
	auto state = std::make_unique<PlayerStateAttack>(attackName);
	state->UpdateAttackData();

	states_[attackName] = std::move(state);
}

void PlayerCombat::DrawAttackDataEditor(PlayerStateAttack* attack)
{
#ifdef _DEBUG
	const char* attackName = attack->name_.c_str();

	int32_t& pointCount = global_->GetValueRef<int32_t>(attackName, "PointCount");

	for (int32_t i = 0; i < pointCount; ++i) {
		Vector3& point = global_->GetValueRef<Vector3>(attackName, "ControlPoint_" + std::to_string(i));
		ImGui::DragFloat3(("P" + std::to_string(i)).c_str(), &point.x, 0.01f);
	}

	if (ImGui::Button("Add Point")) {
		global_->AddItem(attackName, "ControlPoint_" + std::to_string(pointCount), Vector3{});
		++pointCount;
	}

	if (ImGui::Button("Remove Last") && pointCount > 0) {
		--pointCount;
		global_->RemoveItem(attackName, "ControlPoint_" + std::to_string(pointCount));
	}
	ImGui::Separator();

	// 移動系
	ImGui::DragFloat3("Move Speed", &global_->GetValueRef<Vector3>(attackName, "MoveSpeed").x, 0.01f);
	ImGui::DragFloat3("KnockBack Speed", &global_->GetValueRef<Vector3>(attackName, "KnockBackSpeed").x, 0.01f);

	ImGui::Separator();

	// タイマー系
	ImGui::DragFloat("Total Duration", &global_->GetValueRef<float>(attackName, "TotalDuration"), 0.01f);
	ImGui::DragFloat("Pre Delay", &global_->GetValueRef<float>(attackName, "PreDelay"), 0.01f);
	ImGui::DragFloat("Attack Duration", &global_->GetValueRef<float>(attackName, "AttackDuration"), 0.01f);
	ImGui::DragFloat("Post Delay", &global_->GetValueRef<float>(attackName, "PostDelay"), 0.01f);
	ImGui::DragFloat("Next Attack Delay", &global_->GetValueRef<float>(attackName, "NextAttackDelay"), 0.01f);

	ImGui::Separator();

	// その他
	ImGui::Checkbox("Draw Debug Control Points", &global_->GetValueRef<bool>(attackName, "DrawDebugControlPoints"));

	ImGui::DragFloat("Damage", &global_->GetValueRef<float>(attackName, "Damage"), 0.01f);

	ImGui::DragFloat("HitStopTime", &global_->GetValueRef<float>(attackName, "HitStopTime"), 0.01f);
	ImGui::DragFloat("HitStopIntensity", &global_->GetValueRef<float>(attackName, "HitStopIntensity"), 0.01f);

	// 攻撃を受けた側に送る情報
	ImGui::Text("ReactionType:");
	ImGui::SameLine();
	ImGui::RadioButton("HitStun", &global_->GetValueRef<int32_t>(attackName, "ReactionType"), 0);
	ImGui::SameLine();
	ImGui::RadioButton("Knockback", &global_->GetValueRef<int32_t>(attackName, "ReactionType"), 1);
	ImGui::SameLine();
	ImGui::RadioButton("Launch", &global_->GetValueRef<int32_t>(attackName, "ReactionType"), 2);

	// ノックバック＆打ち上げ共通
	ImGui::DragFloat("ImpulseForce", &global_->GetValueRef<float>(attackName, "ImpulseForce"), 0.01f);
	ImGui::DragFloat("UpwardRatio", &global_->GetValueRef<float>(attackName, "UpwardRatio"), 0.01f);
	// 吹っ飛び用
	ImGui::DragFloat("TorqueForce", &global_->GetValueRef<float>(attackName, "TorqueForce"), 0.01f);
	// のけぞり用
	ImGui::DragFloat("StunTime", &global_->GetValueRef<float>(attackName, "StunTime"), 0.01f);

	// 攻撃時に地上にいるかの判定
	ImGui::Separator();

	ImGui::Text("Posture:");
	ImGui::SameLine();
	ImGui::RadioButton("Stand", &global_->GetValueRef<int32_t>(attackName, "AttackPosture"), 0);
	ImGui::SameLine();
	ImGui::RadioButton("Air", &global_->GetValueRef<int32_t>(attackName, "AttackPosture"), 1);

	//// === 派生攻撃インデックス ===
	//ImGui::Separator();
	//ImGui::Text("Next Attacks");

	//int32_t& nextAttackCount = global_->GetValueRef<int32_t>(attack->name_, "NextAttackCount");
	//ImGui::DragInt("Next Attack Count", &nextAttackCount, 1, 0, 5);
	//nextAttackCount = std::clamp(nextAttackCount, 0, 3);

	//// 全攻撃名リストの取得
	//std::vector<std::string> attackNames;
	//for (auto& [name, state] : states_) {
	//	if (dynamic_cast<PlayerStateAttack*>(state.get())) {
	//		attackNames.push_back(name);
	//	}
	//}
	//std::vector<const char*> cstrs;
	//for (const auto& name : attackNames) {
	//	cstrs.push_back(name.c_str());
	//}

	//// 複数の派生攻撃を選択
	//for (int i = 0; i < nextAttackCount; ++i) {
	//	std::string key = "NextAttackIndex_" + std::to_string(i);
	//	int32_t& index = global_->GetValueRef<int32_t>(attack->name_, key);
	//	if (index < 0 || index >= (int32_t)attackNames.size()) {
	//		index = 0; // 範囲外防止
	//	}
	//	std::string label = "Next Attack " + std::to_string(i);
	//	ImGui::Combo(label.c_str(), &index, cstrs.data(), static_cast<int>(cstrs.size()));
	//}

	DrawAttackNodeEditor(attackGraph_[attackName]);

	ImGui::Separator();

	if (ImGui::Button("Save")) {
		global_->SaveFile("PlayerAttack", attackName);
		std::string message = std::format("{}.json saved.", attackName);
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
	}
#endif // IMGUI
}

AttackNode PlayerCombat::LoadAttackNode(const std::string& attackName)
{
	AttackNode node;
	node.name = attackName;

	int count = global_->GetValueRef<int>(attackName, "NextAttackCount");

	for (int i = 0; i < count; ++i) {
		std::string key = "NextAttack_" + std::to_string(i);
		node.nextAttacks.push_back(global_->GetValueRef<std::string>(attackName, key));
	}

	return node;
}

void PlayerCombat::DrawAttackNodeEditor(AttackNode& node)
{
	ImGui::Text("Attack : %s", node.name.c_str());
	ImGui::Separator();

	// 派生先一覧
	for (size_t i = 0; i < node.nextAttacks.size(); ++i) {
		ImGui::BulletText("-> %s", node.nextAttacks[i].c_str());
	}

	ImGui::Separator();

	static int selectedNextIndex = 0;

	if (ImGui::BeginCombo("Add Next Attack", "Select")) {
		for (auto& [name, _] : attackGraph_) {
			bool selected = false;
			if (ImGui::Selectable(name.c_str(), selected)) {
				node.nextAttacks.push_back(name);
			}
		}
		ImGui::EndCombo();
	}

	for (size_t i = 0; i < node.nextAttacks.size(); ++i) {
		ImGui::PushID((int)i);
		ImGui::Text("%s", node.nextAttacks[i].c_str());
		ImGui::SameLine();
		if (ImGui::Button("X")) {
			node.nextAttacks.erase(node.nextAttacks.begin() + i);
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}


	for (auto& [name, node] : attackGraph_) {

		// 派生数
		global_->SetValue(name, "NextAttackCount", static_cast<int32_t>(node.nextAttacks.size()));

		// 派生先
		for (size_t i = 0; i < node.nextAttacks.size(); ++i) {
			std::string key = "NextAttack_" + std::to_string(i);
			global_->SetValue(name, key, node.nextAttacks[i]);
		}

		// 余分な古いデータ削除（重要）
		global_->RemoveItem(name, "NextAttack_");
	}
}

//void PlayerCombat::CreateEditorNode(const std::string& attackName)
//{
//	static NodeID nodeIdGen = 1;
//	static PinID  pinIdGen = 100;
//
//	AttackNodeEditorData data{};
//	data.nodeId = nodeIdGen++;
//	data.inputPin = pinIdGen++;
//
//	// 出力ピンは派生数分
//	for (size_t i = 0; i < attackGraph_[attackName].nextAttacks.size(); ++i) {
//		data.outputPins.push_back(pinIdGen++);
//	}
//
//	data.position = ImVec2(100, 100);
//
//	editorData_[attackName] = data;
//}
//
//void PlayerCombat::DrawAttackNode(const std::string& name)
//{
//	//auto& node = attackGraph_[name];
//	//auto& ed = editorData_[name];
//
//	//ax::NodeEditor::BeginNode(ed.nodeId);
//
//	//ImGui::Text("%s", name.c_str());
//	//ImGui::Separator();
//
//	//// Input
//	//ax::NodeEditor::BeginPin(ed.inputPin, ax::NodeEditor::PinKind::Input);
//	//ImGui::Text("In");
//	//ax::NodeEditor::EndPin();
//
//	//// Outputs
//	//for (size_t i = 0; i < ed.outputPins.size(); ++i) {
//	//	ax::NodeEditor::BeginPin(ed.outputPins[i], ax::NodeEditor::PinKind::Output);
//	//	ImGui::Text("Next %zu", i);
//	//	ax::NodeEditor::EndPin();
//	//}
//
//	//ax::NodeEditor::EndNode();
//}
