#include "PlayerCombat.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"

void PlayerCombat::Initialize(Player* player)
{
	player_ = player;

	CreateState();

	attackPlayer_ = std::make_unique<AttackPlayer>();
	attackPlayer_->SetPlayer(player);

	std::vector<PlayerStateAttack*> list;
	for (auto& [name, state] : states_) {
		list.push_back(state.get());
	}
	attackPlayer_->SetAttacks(list);
}

void PlayerCombat::Update(float deltaTime)
{
	// 毎フレーム
	attackPlayer_->Update(DeltaTime::GetDeltaTime());
	attackPlayer_->DrawImGui();

	DrawAttackDerivativeEditorUI();

	DrawAttackDataEditorUI();
	for (auto& state : states_) {
		state.second->UpdateAttackData();
	}

	if (currentState_.empty()) return;

	auto& top = currentState_.back();

	top->Update(*player_, deltaTime);

	if (top->IsFinished()) {
		top->Exit(*player_);
		currentState_.pop_back();
	}
}

void PlayerCombat::Draw()
{
	for (auto& state : states_) {
		state.second->DrawControlPoints(*player_);
	}
}

void PlayerCombat::RequestAttack(AttackType type)
{
	//switch (type) {
	//case AttackType::Normal:
	//	ChangeState("AttackComboA1");
	//	break;
	//case AttackType::RoundUp:
	//	ChangeState("AttackHighTime");
	//	break;
	//case AttackType::LungeThrust:

	//	break;
	//case AttackType::Air:
	//	ChangeState("AttackAerialRave1");
	//}
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

void PlayerCombat::ExecuteCommand(const PlayerCommand& command)
{
	// 攻撃の入力か確認
	if (command.action != PlayerAction::Attack) return;

	// 攻撃がない場合一段目攻撃を設定 存在したらその攻撃を派生させる
	if (currentState_.empty()) {
		if (player_->IsLockOn()) {
			if (player_->GetOnGround()) {
				if (command.stickDir.y <= -0.7f) {
					ChangeState("AttackHighTime");
					return;
				}
			}
		}

		// ここまで来て攻撃が見つからなかったら通常
		if (player_->GetOnGround()) {
			ChangeState("AttackComboA1");
		} else {
			ChangeState("AttackAerialRave1");
		}
	} else {
		auto& top = currentState_.back();
		auto req = top->ExecuteCommand(*player_, command);

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

	// 入力系
	ImGui::Checkbox("IsRootAttack", &global_->GetValueRef<bool>(attackName, "RootAttackFlag"));
	ImGui::Checkbox("IsAir", &global_->GetValueRef<bool>(attackName, "IsAir"));

	ImGui::Separator();

	const char* buttonLabels[] = { "None", "X", "Y" };
	ImGui::Combo("Button", &global_->GetValueRef<int32_t>(attackName, "ButtonIndex"), buttonLabels, IM_ARRAYSIZE(buttonLabels));

	ImGui::Checkbox("RequireLockOn", &global_->GetValueRef<bool>(attackName, "LockOnFlag"));

	if (global_->GetValueRef<bool>(attackName, "LockOnFlag"))
	{
		const char* dirLabels[] = {
			"None",
			"To Enemy",
			"Away From Enemy",
			"Any"
		};
		ImGui::Combo("Stick Direction", &global_->GetValueRef<int32_t>(attackName, "DirIndex"), dirLabels, IM_ARRAYSIZE(dirLabels));
	}

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

	for (auto& next : node.nextAttacks) {
		ImGui::BulletText("-> %s", next.c_str());
	}
}

void PlayerCombat::DrawAttackDerivativeEditorUI()
{
#ifdef _DEBUG
	static std::string selectedAttack;

	ImGui::Begin("Attack Derivative Editor");

	// --- 左：攻撃一覧 ---
	ImGui::BeginChild("AttackList", ImVec2(200, 0), true);
	for (auto& [name, node] : attackGraph_)
	{
		if (ImGui::Selectable(name.c_str(), selectedAttack == name))
		{
			selectedAttack = name;
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// --- 右：派生先設定 ---
	ImGui::BeginChild("DerivativeSetting", ImVec2(0, 0), true);

	if (!selectedAttack.empty())
	{
		AttackNode& node = attackGraph_[selectedAttack];

		ImGui::Text("Selected Attack: %s", selectedAttack.c_str());
		ImGui::Separator();

		for (auto& [targetName, targetNode] : attackGraph_)
		{
			if (targetName == selectedAttack)
				continue;

			bool hasLink = std::find(node.nextAttacks.begin(), node.nextAttacks.end(), targetName) != node.nextAttacks.end();

			if (ImGui::Checkbox(targetName.c_str(), &hasLink)) {
				if (hasLink) {
					node.nextAttacks.push_back(targetName);
				} else {
					std::erase(node.nextAttacks, targetName);
				}
			}
		}
	}

	ImGui::EndChild();

	ImGui::End();
#endif // DEBUG
}
