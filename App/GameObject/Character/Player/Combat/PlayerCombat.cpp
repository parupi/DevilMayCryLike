#include "PlayerCombat.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include <Utility/DeltaTime.h>
#ifdef _DEBUG
#endif

namespace {
	// 納刀モーションの攻撃名。見た目だけの動きなので、ルート攻撃でいつでも割り込めるようにしている
	constexpr const char* kSheatheName = "Sheathe";
	// 攻撃の硬直中に押された、別のボタンのルート攻撃を覚えておく時間[秒]
	constexpr float kRootAttackBufferTime = 0.3f;

	// エディタに出す、攻撃に割り当てたボタンの名前
	const char* ButtonLabel(InputButton button) {
		switch (button) {
		case InputButton::X: return "X";
		case InputButton::Y: return "Y";
		default: return "Any";
		}
	}
}

void PlayerCombat::Initialize(Player* player) {
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

void PlayerCombat::Update(float deltaTime) {
	// 毎フレーム
	attackPlayer_->Update(DeltaTime::GetDeltaTime());

	// カウンターの受付とクールタイムは実時間で数える（ジャスト回避のスロー中に伸びないように）
	const float realDelta = DeltaTime::GetDeltaTime();
	if (counterWindowTimer_ > 0.0f) {
		counterWindowTimer_ -= realDelta;
	}
	if (counterCooldownTimer_ > 0.0f) {
		counterCooldownTimer_ -= realDelta;
	}
	// ルート攻撃の先行入力はゲーム内の時間で数える（ヒットストップ中に消えないように）
	if (pendingRootTimer_ > 0.0f) {
		pendingRootTimer_ -= deltaTime;
	}

	// エディタのUIは App/Editor/Windows/AttackEditorWindow.cpp から呼ばれる
	for (auto& [name, state] : states_) {
		state->UpdateAttackData();
		auto& node = attackGraph_[name];
		node.isRootAttack = global_->GetValueRef<bool>(name, "RootAttackFlag");
		node.isAir = global_->GetValueRef<bool>(name, "IsAir");
		node.condition.button = static_cast<InputButton>(global_->GetValueRef<int32_t>(name, "ButtonIndex"));
		node.condition.requireLockOn = global_->GetValueRef<bool>(name, "LockOnFlag");
		node.condition.stick = static_cast<StickDirection>(global_->GetValueRef<int32_t>(name, "DirIndex"));
		node.condition.requireCounter = global_->GetValueRef<bool>(name, "CounterFlag");
	}
	// コンボのリセット処理
	if (waitingForNextCombo_) {

		comboResetTimer_ -= deltaTime;

		if (comboResetTimer_ <= 0.0f) {
			waitingForNextCombo_ = false;

			// 納刀モーション
			AddState(kSheatheName);
		}
	}

	if (currentState_.empty()) return;

	auto& top = currentState_.back();

	top->Update(*player_, deltaTime);

	// 先行入力が Cancel フェーズ突入時に発火していたら処理する
	if (top->HasPendingRequest()) {
		// コンボの続きが出るので、別のボタンのルート攻撃の先行入力は捨てる
		pendingRootTimer_ = 0.0f;

		auto req = top->ConsumePendingRequest();
		if (req.type == AttackRequest::ChangeAttack && states_.count(req.nextAttack)) {
			// AddState を使うと push_back 後も top が旧ステートを指すため
			// pop_back が新ステートを誤って消す。直接 Exit→pop→Enter→push する。
			top->Exit(*player_);
			currentState_.pop_back();
			auto* next = states_[req.nextAttack].get();
			next->Enter(*player_);
			currentState_.push_back(next);
		} else if (req.type == AttackRequest::Jump) {
			player_->ChangeState("Jump");
		}
		return; // top は無効になるので IsFinished チェックをスキップ
	}

	// 硬直中に押された別のボタンのルート攻撃を、割り込めるようになった時点で出す
	if (pendingRootTimer_ > 0.0f && IsReadyForRootAttack(*top)) {
		const PlayerCommand command = pendingRootCommand_;
		pendingRootTimer_ = 0.0f;
		if (const AttackNode* root = FindRootAttack(command)) {
			InterruptCombat();
			StartRootAttack(*root);
			return; // top は無効になるので IsFinished チェックをスキップ
		}
	}

	// 攻撃が終了していたらスタックから抜ける
	if (top->IsFinished()) {
		// スタックから抜ける前に、もしこの攻撃が派生可能だったら、次の攻撃を待つ状態に入る
		bool wasBranching = top->HasBranch(*player_);

		// 現在の攻撃の名前を保持
		auto& currentAttackName = top->GetAttackName();

		top->Exit(*player_);
		currentState_.pop_back();

		// 次の攻撃を待つ状態に入る
		if (wasBranching) {
			waitingForNextCombo_ = true;
			comboResetTimer_ = 0.3f;
		} else {
			// 納刀モーションでなければ
			if (currentAttackName != kSheatheName) {
				// 少し待ってから納刀モーションに遷移する
				waitingForNextCombo_ = true;
				comboResetTimer_ = 0.1f;
			}
		}
	}
}

void PlayerCombat::Draw() {
	for (auto& state : states_) {
		state.second->DrawControlPoints(*player_);
	}
}

void PlayerCombat::InterruptCombat() {
	for (auto* state : currentState_) {
		state->Exit(*player_);
	}
	currentState_.clear();
	waitingForNextCombo_ = false;
	comboResetTimer_ = 0.0f;
	pendingRootTimer_ = 0.0f;
}

void PlayerCombat::AddState(const std::string& stateName) {
	// 攻撃中でなければそのまま遷移
	if (currentState_.empty()) {
		auto it = states_[stateName].get();
		it->Enter(*player_);
		currentState_.push_back(it);
		return;
	}
	// 攻撃中であっても、現在の攻撃が割り込み可能であれば遷移
	if (currentState_.back()->CanBeInterrupted()) {
		auto it = states_[stateName].get();
		it->Enter(*player_);
		currentState_.back()->OnInterrupted(*player_);
		currentState_.push_back(it);
		return;
	}
}

void PlayerCombat::ExecuteCommand(const PlayerCommand& command) {
	if (command.action != PlayerAction::Attack) return;
	waitingForNextCombo_ = false;

	if (currentState_.empty()) {
		if (const AttackNode* root = FindRootAttack(command)) {
			StartRootAttack(*root);
		}
		return;
	}

	PlayerStateAttack* top = currentState_.back();

	// 今の攻撃から派生できない別のボタンなら、そのボタンのルート攻撃として扱う。
	// 以前は攻撃が終わりきるまで入力を捨てていて、終わった直後に納刀モーションが始まるとそこでも捨てていたので、
	// Y の攻撃の後に X のルート攻撃を押しても、ほとんど出なかった
	if (CanStartRootAttackOver(*top, command.button)) {
		if (IsReadyForRootAttack(*top)) {
			if (const AttackNode* root = FindRootAttack(command)) {
				InterruptCombat();
				StartRootAttack(*root);
			}
		} else {
			// 硬直中なら覚えておき、割り込めるようになった時点で Update が出す
			pendingRootCommand_ = command;
			pendingRootTimer_ = kRootAttackBufferTime;
		}
		return;
	}

	auto req = top->ExecuteCommand(*player_, command);

	switch (req.type) {
	case AttackRequest::Jump:
		player_->ChangeState("Jump");
		break;
	case AttackRequest::Air:
		player_->ChangeState("Air");
		break;
	case AttackRequest::ChangeAttack:
		AddState(req.nextAttack);
		break;
	case AttackRequest::None:
		break;
	}
}

const AttackNode* PlayerCombat::FindRootAttack(const PlayerCommand& command) const {
	bool onGround = player_->GetOnGround();
	bool lockedOn = player_->IsLockOn();

	const AttackNode* bestMatch = nullptr;
	int bestScore = -1;

	for (const auto& [name, node] : attackGraph_) {
		if (!node.isRootAttack) continue;

		// 地上/空中が一致しなければスキップ
		if (node.isAir == onGround) continue;

		// ロックオン必須なのにロックオンしていなければスキップ
		if (node.condition.requireLockOn && !lockedOn) continue;

		// ジャスト回避の直後にしか出せない攻撃（カウンター）は、受付中でなければスキップ
		if (node.condition.requireCounter && !IsCounterWindowOpen()) continue;

		// ボタンが指定されていてコマンドと違えばスキップ
		if (node.condition.button != InputButton::None && node.condition.button != command.button) continue;

		// スティック方向チェック
		bool stickMatch = false;
		switch (node.condition.stick) {
		case StickDirection::ToEnemy:
			stickMatch = command.stickDir.y >= 0.7f;
			break;
		case StickDirection::AwayFromEnemy:
			stickMatch = command.stickDir.y <= -0.7f;
			break;
		case StickDirection::None:
		case StickDirection::Any:
			stickMatch = true;
			break;
		}
		if (!stickMatch) continue;

		// 条件が多いほど優先度が高い。カウンターは出せる場面が限られるので一番優先する
		int score = 0;
		if (node.condition.requireCounter) score += 8;
		if (node.condition.requireLockOn) score += 4;
		if (node.condition.stick == StickDirection::ToEnemy || node.condition.stick == StickDirection::AwayFromEnemy) score += 2;
		if (node.condition.button != InputButton::None) score += 1;

		if (score > bestScore) {
			bestScore = score;
			bestMatch = &node;
		}
	}

	return bestMatch;
}

void PlayerCombat::StartRootAttack(const AttackNode& node) {
	pendingRootTimer_ = 0.0f;

	// カウンターを出したら受付を閉じて、クールタイムに入る
	if (node.condition.requireCounter) {
		counterWindowTimer_ = 0.0f;
		counterCooldownTimer_ = player_->GetDodgeParams().counterCooldown;
	}
	AddState(node.name);
}

bool PlayerCombat::CanStartRootAttackOver(const PlayerStateAttack& attack, InputButton button) const {
	// 今の攻撃から派生できるボタンなら、コンボの続きの方を優先する
	if (attack.CanBranchWith(*player_, button)) return false;

	// 納刀モーションは見た目だけなので、どのボタンでも割り込める
	if (attack.GetAttackName() == kSheatheName) return true;

	// 続きの無い技を同じボタンで連打しても、ルート攻撃から振り直さない（従来どおり、終わるまで待つ）
	const auto it = attackGraph_.find(attack.GetAttackName());
	if (it == attackGraph_.end()) return false;
	return it->second.condition.button != button;
}

bool PlayerCombat::IsReadyForRootAttack(const PlayerStateAttack& attack) const {
	return attack.CanBeInterrupted() || attack.GetAttackName() == kSheatheName;
}

void PlayerCombat::OpenCounterWindow(float seconds) {
	// 立て続けにカウンターを出せないよう、クールタイム中は受付を開かない
	if (counterCooldownTimer_ > 0.0f) return;
	counterWindowTimer_ = seconds;
}

void PlayerCombat::CreateState() {
	auto list = global_->GetGroupNames("PlayerAttack");
	for (std::string name : list) {
		global_->LoadFile("PlayerAttack", name);
		states_[name] = std::make_unique<PlayerStateAttack>(name);
		states_[name]->UpdateAttackData();
		attackGraph_[name] = LoadAttackNode(name);
	}
}

#ifdef _DEBUG
void PlayerCombat::DrawAttackDataEditorUI() {
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

}

void PlayerCombat::DrawRootAttackCheck(const std::string& attackName) {
	if (!ImGui::CollapsingHeader("Check : 今ルート攻撃として出せるか", ImGuiTreeNodeFlags_DefaultOpen)) return;

	const auto it = attackGraph_.find(attackName);
	if (it == attackGraph_.end()) return;
	const AttackNode& node = it->second;

	auto row = [](bool ok, const std::string& text) {
		const ImVec4 color = ok ? ImVec4(0.55f, 0.90f, 0.55f, 1.0f) : ImVec4(1.0f, 0.45f, 0.40f, 1.0f);
		ImGui::TextColored(color, "%s  %s", ok ? "OK" : "NG", text.c_str());
	};

	// ── 出す条件（FindRootAttack と同じ並び） ──
	row(node.isRootAttack, "IsRootAttack が ON（OFF だとコンボの派生でしか出ない）");

	const bool onGround = player_->GetOnGround();
	row(node.isAir != onGround, std::format("地上/空中が合っている（今: {} / この攻撃: {}用。IsAir で切り替え）",
		onGround ? "地上" : "空中", node.isAir ? "空中" : "地上"));

	if (node.condition.requireLockOn) {
		row(player_->IsLockOn(), "ロックオンしている（RequireLockOn が ON）");
	}
	if (node.condition.requireCounter) {
		row(IsCounterWindowOpen(), "カウンター受付中（Require Just Dodge が ON。ジャスト回避の直後だけ出る）");
	}

	switch (node.condition.button) {
	case InputButton::X: ImGui::TextDisabled("      押すボタン: X（キーボードは K）"); break;
	case InputButton::Y: ImGui::TextDisabled("      押すボタン: Y（キーボードは J）"); break;
	default: ImGui::TextDisabled("      押すボタン: X / Y どちらでも"); break;
	}
	switch (node.condition.stick) {
	case StickDirection::ToEnemy: ImGui::TextDisabled("      スティック（移動キー）を前に倒しながら押す（Stick Direction）"); break;
	case StickDirection::AwayFromEnemy: ImGui::TextDisabled("      スティック（移動キー）を後ろに倒しながら押す（Stick Direction）"); break;
	default: break;
	}

	// 同じ入力で、もっと条件の多い攻撃が選ばれてしまっていないか
	if (node.isRootAttack) {
		PlayerCommand command{};
		command.action = PlayerAction::Attack;
		command.button = (node.condition.button == InputButton::None) ? InputButton::X : node.condition.button;
		if (node.condition.stick == StickDirection::ToEnemy) command.stickDir = { 0.0f, 1.0f };
		if (node.condition.stick == StickDirection::AwayFromEnemy) command.stickDir = { 0.0f, -1.0f };
		const AttackNode* chosen = FindRootAttack(command);
		if (chosen && chosen->name != attackName) {
			row(false, std::format("同じ入力では「{}」が選ばれる（条件の多い攻撃が優先）", chosen->name));
		}
	}

	// ── データ ──
	const float total = global_->GetValueRef<float>(attackName, "TotalDuration");
	const float pre = global_->GetValueRef<float>(attackName, "PreDelay");
	const float active = global_->GetValueRef<float>(attackName, "AttackDuration");
	const float post = global_->GetValueRef<float>(attackName, "PostDelay");
	const int32_t pointCount = global_->GetValueRef<int32_t>(attackName, "PointCount");

	row(total > 0.0f, "Total Duration が 0 より大きい（0 だと出た瞬間に終わる）");
	row(active > 0.0f, "Attack Duration が 0 より大きい（0 だと当たり判定が出ない）");
	row(pointCount >= 4, std::format("制御点が4つ以上ある（今 {} 個。足りないと武器が振られない）", pointCount));
	if (!node.nextAttacks.empty() && pre + active + post >= total) {
		row(false, std::format("Pre + Attack + Post Delay（{:.2f}）が Total Duration（{:.2f}）以上なので、派生を受け付ける時間が無い",
			pre + active + post, total));
	}
}
#endif // _DEBUG

void PlayerCombat::AddAttackState(const std::string& attackName) {
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

	// 攻撃グラフにも名前付きで登録する。
	// 以前は states_ にしか足しておらず、Update がノードを名前の空のまま作っていたので、
	// 追加した攻撃をルート攻撃にして出すと、空の名前でステートを探して落ちていた
	attackGraph_[attackName] = LoadAttackNode(attackName);

	// 攻撃の試し再生（AttackPlayer）の一覧にも入れる
	std::vector<PlayerStateAttack*> list;
	for (auto& [name, attack] : states_) {
		list.push_back(attack.get());
	}
	attackPlayer_->SetAttacks(list);
}

void PlayerCombat::DrawAttackDataEditor([[maybe_unused]] PlayerStateAttack* attack) {
#ifdef _DEBUG
	const char* attackName = attack->name_.c_str();

	// 出ないときに理由を探せるよう、一番上に出す
	DrawRootAttackCheck(attack->name_);
	ImGui::Separator();

	int32_t& pointCount = global_->GetValueRef<int32_t>(attackName, "PointCount");

	for (int32_t i = 0; i < pointCount; ++i) {

		const std::string rotationKey = "ControlRotation_" + std::to_string(i);

		if (!global_->HasItem(attackName, rotationKey)) {
			global_->AddItem(attackName, rotationKey, Vector3{});
		}
	}

	for (int32_t i = 0; i < pointCount; ++i) {

		ImGui::PushID(i);

		//=========================
		// Position
		//=========================
		Vector3& point = global_->GetValueRef<Vector3>(attackName, "ControlPoint_" + std::to_string(i));

		ImGui::DragFloat3("Position", &point.x, 0.01f);

		//=========================
		// Rotation (Euler)
		//=========================
		Vector3& rotation = global_->GetValueRef<Vector3>(attackName, "ControlRotation_" + std::to_string(i));

		ImGui::DragFloat3("Rotation", &rotation.x, 1.0f);

		ImGui::Separator();

		ImGui::PopID();
	}

	if (ImGui::Button("Add Point")) {
		global_->AddItem(attackName, "ControlPoint_" + std::to_string(pointCount), Vector3{});
		global_->AddItem(attackName, "ControlRotation_" + std::to_string(pointCount), Vector3{});

		++pointCount;
	}

	if (ImGui::Button("Remove Last") && pointCount > 0) {
		--pointCount;
		global_->RemoveItem(attackName, "ControlPoint_" + std::to_string(pointCount));
		global_->RemoveItem(attackName, "ControlRotation_" + std::to_string(pointCount));
	}
	ImGui::Separator();

	// 移動系
	ImGui::DragFloat3("Move Speed", &global_->GetValueRef<Vector3>(attackName, "MoveSpeed").x, 0.01f);

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

	const char* buttonLabels[] = {"None", "X", "Y"};
	ImGui::Combo("Button", &global_->GetValueRef<int32_t>(attackName, "ButtonIndex"), buttonLabels, IM_ARRAYSIZE(buttonLabels));

	ImGui::Checkbox("RequireLockOn", &global_->GetValueRef<bool>(attackName, "LockOnFlag"));

	// スティック方向はロックオンしていなくても判定に効く。
	// 以前は RequireLockOn が ON のときしか出しておらず、OFF にすると設定が見えないまま残って、攻撃が出なくなることがあった
	const char* dirLabels[] = {
		"None",
		"To Enemy",
		"Away From Enemy",
		"Any"
	};
	ImGui::Combo("Stick Direction", &global_->GetValueRef<int32_t>(attackName, "DirIndex"), dirLabels, IM_ARRAYSIZE(dirLabels));
	ImGui::SetItemTooltip("To Enemy = スティック（移動キー）を前に倒しながら押す\n"
		"Away From Enemy = 後ろに倒しながら押す\n"
		"ロックオンしていなくても効く");

	ImGui::Checkbox("Require Just Dodge (Counter)", &global_->GetValueRef<bool>(attackName, "CounterFlag"));
	ImGui::SetItemTooltip("ジャスト回避の直後だけ出せる攻撃（カウンター）にする。\n"
		"受付時間とクールタイムは Dodge / Dash ウィンドウの「ジャスト回避」で調整する。\n"
		"条件が合えば、同じボタンの他の攻撃より優先される");

	ImGui::Separator();

	// その他
	ImGui::Checkbox("IsMove", &global_->GetValueRef<bool>(attackName, "IsMove"));
	ImGui::Checkbox("Draw Debug Control Points", &global_->GetValueRef<bool>(attackName, "DrawDebugControlPoints"));

	ImGui::DragFloat("Damage", &global_->GetValueRef<float>(attackName, "Damage"), 0.01f);

	ImGui::DragFloat("HitStopTime", &global_->GetValueRef<float>(attackName, "HitStopTime"), 0.01f);
	ImGui::DragFloat("HitStopIntensity", &global_->GetValueRef<float>(attackName, "HitStopIntensity"), 0.01f);

	// 攻撃の強さでヒットストップ中のタイムスケールが変わる
	int32_t& hitStopStrength = global_->GetValueRef<int32_t>(attackName, "HitStopStrength");
	ImGui::Text("HitStopStrength:");
	ImGui::SameLine();
	ImGui::RadioButton("Light", &hitStopStrength, static_cast<int32_t>(HitStopStrength::Light));
	ImGui::SameLine();
	ImGui::RadioButton("Medium", &hitStopStrength, static_cast<int32_t>(HitStopStrength::Medium));
	ImGui::SameLine();
	ImGui::RadioButton("Heavy", &hitStopStrength, static_cast<int32_t>(HitStopStrength::Heavy));
	ImGui::Text("  -> TimeScale: %.2f", HitStop::ToTimeScale(HitStop::ToStrength(hitStopStrength)));

	// ── 攻撃を受けた側に送るノックバック情報（仕様書 §3〜§8）──
	ImGui::SeparatorText("Knockback");
	ImGui::Text("ReactionType:");
	ImGui::SameLine();
	ImGui::RadioButton("HitStun", &global_->GetValueRef<int32_t>(attackName, "ReactionType"), 0);
	ImGui::SameLine();
	ImGui::RadioButton("Knockback", &global_->GetValueRef<int32_t>(attackName, "ReactionType"), 1);
	ImGui::SameLine();
	ImGui::RadioButton("Launch", &global_->GetValueRef<int32_t>(attackName, "ReactionType"), 2);
	ImGui::SetItemTooltip("HitStun = 少し後退するだけ（通常攻撃）\n"
		"Knockback = 大きく後退する（強攻撃）\n"
		"Launch = 打ち上げて空中コンボへ繋ぐ\n"
		"種類ごとに水平／上方向の効き方が変わるので、切り替えても数値を付け直さなくてよい");

	// ノックバック＆打ち上げ共通
	const float impulseForce = global_->GetValueRef<float>(attackName, "ImpulseForce");
	ImGui::DragFloat("ImpulseForce", &global_->GetValueRef<float>(attackName, "ImpulseForce"), 0.01f);
	ImGui::SetItemTooltip("水平方向の初速[m/s]。この速さから時間をかけて 0 まで減速する");
	ImGui::DragFloat("UpwardRatio", &global_->GetValueRef<float>(attackName, "UpwardRatio"), 0.01f);
	ImGui::SetItemTooltip("ImpulseForce に対する上方向の割合。Launch はここを高めにする");
	ImGui::Text("  -> 上方向の初速: %.2f m/s", impulseForce * global_->GetValueRef<float>(attackName, "UpwardRatio"));
	// 吹っ飛び用
	ImGui::DragFloat("TorqueForce", &global_->GetValueRef<float>(attackName, "TorqueForce"), 0.01f);
	ImGui::SetItemTooltip("吹き飛び・打ち上げ中に体が回る速さ[度/秒]");
	// のけぞり用
	ImGui::DragFloat("StunTime", &global_->GetValueRef<float>(attackName, "StunTime"), 0.01f);
	ImGui::SetItemTooltip("のけぞりで動けなくなる時間[秒]。吹き飛び・打ち上げは着地するまでなので効かない");

	// 時間と減速（仕様書 §7）
	float& knockbackDuration = global_->GetValueRef<float>(attackName, "KnockbackDuration");
	ImGui::DragFloat("Knockback Duration", &knockbackDuration, 0.01f, 0.0f, 5.0f, "%.2f 秒");
	ImGui::SetItemTooltip("ノックバックが続く時間。0 なら ReactionType ごとの既定値\n"
		"（HitStun 0.18秒 / Knockback 0.45秒 / Launch 0.35秒）");
	ImGui::DragFloat("Knockback Deceleration", &global_->GetValueRef<float>(attackName, "KnockbackDeceleration"), 0.01f, 0.1f, 10.0f);
	ImGui::SetItemTooltip("減速カーブの鋭さ。1.0 で直線に減速する。\n"
		"大きくすると初速だけ強く出てすぐ止まり、小さくすると長く滑る");
	ImGui::DragFloat("Knockback Max Speed", &global_->GetValueRef<float>(attackName, "KnockbackMaxSpeed"), 0.1f, 0.0f, 100.0f);
	ImGui::SetItemTooltip("合成後の速度の上限[m/s]。0 で無制限。\n"
		"多段ヒットで速度が伸びすぎるときに使う");

	// 向き（仕様書 §4）
	const char* knockbackDirLabels[] = { "Away From Attacker", "Attacker Forward", "Upward", "Toward Attacker" };
	ImGui::Combo("Knockback Direction", &global_->GetValueRef<int32_t>(attackName, "KnockbackDirection"),
		knockbackDirLabels, IM_ARRAYSIZE(knockbackDirLabels));
	ImGui::SetItemTooltip("Away From Attacker = 攻撃者から離れる（基本）\n"
		"Attacker Forward = 横から当てても自分の正面へ飛ばす\n"
		"Upward = 水平には飛ばさず真上へ\n"
		"Toward Attacker = 引き寄せる（コンボ維持用）");

	// 連続ヒット時の合成（仕様書 §8）
	const char* knockbackBlendLabels[] = { "Override", "Additive" };
	ImGui::Combo("Knockback Blend", &global_->GetValueRef<int32_t>(attackName, "KnockbackBlend"),
		knockbackBlendLabels, IM_ARRAYSIZE(knockbackBlendLabels));
	ImGui::SetItemTooltip("ノックバック中にもう一度当たったときの合成方法。\n"
		"Override = 上書き（強い攻撃の反応が分かりやすい）\n"
		"Additive = 加算して Max Speed で頭打ち（多段ヒットと相性がよい）");

	ImGui::Checkbox("Override Velocity", &global_->GetValueRef<bool>(attackName, "KnockbackOverrideVelocity"));
	ImGui::SetItemTooltip("ON なら相手の元の速度を捨てて飛ばす。OFF なら残っている勢いに足す");
	ImGui::Checkbox("Can Launch", &global_->GetValueRef<bool>(attackName, "KnockbackCanLaunch"));
	ImGui::SetItemTooltip("この攻撃で相手を浮かせてよいか。敵ごとの耐性と AND される\n"
		"（OFF にすると Launch を指定しても吹き飛びに落ちる）");

	// ── 多段ヒット・当たり判定 ──
	ImGui::SeparatorText("Hit");
	int32_t& hitCount = global_->GetValueRef<int32_t>(attackName, "HitCount");
	ImGui::DragInt("Hit Count", &hitCount, 0.05f, 1, 10);
	ImGui::SetItemTooltip("攻撃判定が出ている間に何回当たり直すか。\n"
		"Attack Duration を等分し、区切りごとに触れている敵へもう一度当たる（回転攻撃など）");
	hitCount = std::clamp(hitCount, 1, 10);

	ImGui::DragFloat("Hitbox Scale", &global_->GetValueRef<float>(attackName, "HitboxScale"), 0.01f, 0.1f, 10.0f);
	ImGui::SetItemTooltip("武器の当たり判定の大きさの倍率。1.0 で通常");

	if (hitCount >= 2) {
		bool& useFinalHit = global_->GetValueRef<bool>(attackName, "UseFinalHit");
		ImGui::Checkbox("Use Final Hit", &useFinalHit);
		ImGui::SetItemTooltip("最終段だけ下の性能に差し替える。\nそれより前の段は上の Damage / ReactionType などを使う");
		if (useFinalHit) {
			ImGui::Indent();
			ImGui::DragFloat("Final Damage", &global_->GetValueRef<float>(attackName, "FinalDamage"), 0.01f);
			int32_t& finalType = global_->GetValueRef<int32_t>(attackName, "FinalReactionType");
			ImGui::Text("Final ReactionType:");
			ImGui::SameLine();
			ImGui::RadioButton("HitStun##Final", &finalType, 0);
			ImGui::SameLine();
			ImGui::RadioButton("Knockback##Final", &finalType, 1);
			ImGui::SameLine();
			ImGui::RadioButton("Launch##Final", &finalType, 2);
			ImGui::DragFloat("Final ImpulseForce", &global_->GetValueRef<float>(attackName, "FinalImpulseForce"), 0.01f);
			ImGui::DragFloat("Final UpwardRatio", &global_->GetValueRef<float>(attackName, "FinalUpwardRatio"), 0.01f);
			ImGui::DragFloat("Final TorqueForce", &global_->GetValueRef<float>(attackName, "FinalTorqueForce"), 0.01f);
			ImGui::DragFloat("Final StunTime", &global_->GetValueRef<float>(attackName, "FinalStunTime"), 0.01f);
			ImGui::DragFloat("Final HitStopTime", &global_->GetValueRef<float>(attackName, "FinalHitStopTime"), 0.01f);
			ImGui::Unindent();
		}
	}

	// ── 溜め ──
	ImGui::SeparatorText("Charge");
	bool& isCharge = global_->GetValueRef<bool>(attackName, "IsCharge");
	ImGui::Checkbox("IsCharge", &isCharge);
	ImGui::SetItemTooltip("ボタンを押し続けている間、構え（Pre Delay の終わり）で止めて溜める。離すと振る。\n"
		"構えの途中で離したときは、溜め無し（倍率 1.0）で振る");
	if (isCharge) {
		ImGui::DragFloat("Charge Min Time", &global_->GetValueRef<float>(attackName, "ChargeMinTime"), 0.01f, 0.0f, 10.0f, "%.2f 秒");
		ImGui::SetItemTooltip("溜めがこれより短いと倍率 1.0 のまま（軽く押しただけでも最低限の攻撃として出る）");
		ImGui::DragFloat("Charge Max Time", &global_->GetValueRef<float>(attackName, "ChargeMaxTime"), 0.01f, 0.0f, 10.0f, "%.2f 秒");
		ImGui::SetItemTooltip("ここまで溜めると最大倍率。溜めきった瞬間にキャラクターのライトが光る");
		ImGui::DragFloat("Charge Damage Scale", &global_->GetValueRef<float>(attackName, "ChargeDamageScale"), 0.01f, 0.0f, 20.0f);
		ImGui::SetItemTooltip("最大まで溜めたときのダメージ倍率。Min 〜 Max の間は線形に上がる");
		ImGui::DragFloat("Charge Impulse Scale", &global_->GetValueRef<float>(attackName, "ChargeImpulseScale"), 0.01f, 0.0f, 20.0f);
		ImGui::SetItemTooltip("最大まで溜めたときの ImpulseForce（吹き飛ばし・打ち上げの強さ）の倍率");
		ImGui::DragFloat("Charge HitStop Scale", &global_->GetValueRef<float>(attackName, "ChargeHitStopScale"), 0.01f, 0.0f, 20.0f);
		ImGui::SetItemTooltip("最大まで溜めたときの HitStopTime の倍率");
	}

	// ── 無敵 ──
	ImGui::SeparatorText("Invincible");
	ImGui::DragFloat("Invincible Time", &global_->GetValueRef<float>(attackName, "InvincibleTime"), 0.01f, 0.0f, 5.0f, "%.2f 秒");
	ImGui::SetItemTooltip("攻撃の出始めから被弾しない時間。カウンターや溜め攻撃の振り出しに使う");

	// 攻撃時に地上にいるかの判定
	ImGui::Separator();

	ImGui::Text("AttackPosture:");
	ImGui::SameLine();
	ImGui::RadioButton("Stand", &global_->GetValueRef<int32_t>(attackName, "AttackPosture"), 0);
	ImGui::SameLine();
	ImGui::RadioButton("Air", &global_->GetValueRef<int32_t>(attackName, "AttackPosture"), 1);

	DrawAttackNodeEditor(attackName, attackGraph_[attackName]);

	ImGui::Separator();

	if (ImGui::Button("Save")) {
		global_->SaveFile("PlayerAttack", attackName);
		std::string message = std::format("{}.json saved.", attackName);
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
	}
#endif // IMGUI
}

AttackNode PlayerCombat::LoadAttackNode(const std::string& attackName) {
	AttackNode node;
	node.name = attackName;
	node.isRootAttack = global_->GetValueRef<bool>(attackName, "RootAttackFlag");
	node.isAir = global_->GetValueRef<bool>(attackName, "IsAir");
	node.condition.button = static_cast<InputButton>(global_->GetValueRef<int32_t>(attackName, "ButtonIndex"));
	node.condition.requireLockOn = global_->GetValueRef<bool>(attackName, "LockOnFlag");
	node.condition.stick = static_cast<StickDirection>(global_->GetValueRef<int32_t>(attackName, "DirIndex"));
	node.condition.requireCounter = global_->GetValueRef<bool>(attackName, "CounterFlag");

	int count = global_->GetValueRef<int>(attackName, "NextAttackCount");
	for (int i = 0; i < count; ++i) {
		std::string key = "NextAttack_" + std::to_string(i);
		// 派生先の枠は既定で3つ分しか作っていない。項目が無ければ読まずに飛ばす
		if (!global_->HasItem(attackName, key)) continue;
		node.nextAttacks.push_back(global_->GetValueRef<std::string>(attackName, key));
	}

	return node;
}

void PlayerCombat::DrawAttackNodeEditor(const std::string& attackName, AttackNode& node) {
	ImGui::Text("Attack : %s", node.name.c_str());
	ImGui::Separator();

	int count = static_cast<int>(attackGraph_[attackName].nextAttacks.size());

	global_->GetValueRef<int>(attackName, "NextAttackCount") = count;

	for (int i = 0; i < count; ++i) {
		// 派生先は押したボタンごとに選ばれるので、どのボタンで出る派生かも並べて見せる
		auto target = attackGraph_.find(node.nextAttacks[i]);
		const char* button = (target != attackGraph_.end()) ? ButtonLabel(target->second.condition.button) : "?";
		ImGui::BulletText("-> %s  [%s]", node.nextAttacks[i].c_str(), button);

		std::string key = "NextAttack_" + std::to_string(i);
		// 派生先の枠は既定で3つ。Attack Derivative Editor で4つ目以降を付けたときに、
		// 無い項目を読みに行って止まらないよう、足りなければここで作る
		if (!global_->HasItem(attackName, key)) {
			global_->AddItem(attackName, key, std::string(""));
		}
		global_->GetValueRef<std::string>(attackName, key) = node.nextAttacks[i];
	}
}

#ifdef _DEBUG
void PlayerCombat::DrawAttackDerivativeEditorUI() {
	static std::string selectedAttack;


	// --- 左：攻撃一覧 ---
	ImGui::BeginChild("AttackList", ImVec2(200, 0), true);
	for (auto& [name, node] : attackGraph_) {
		if (ImGui::Selectable(name.c_str(), selectedAttack == name)) {
			selectedAttack = name;
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// --- 右：派生先設定 ---
	ImGui::BeginChild("DerivativeSetting", ImVec2(0, 0), true);

	if (!selectedAttack.empty()) {
		AttackNode& node = attackGraph_[selectedAttack];

		ImGui::Text("Selected Attack: %s", selectedAttack.c_str());
		ImGui::Separator();

		for (auto& [targetName, targetNode] : attackGraph_) {
			if (targetName == selectedAttack)
				continue;

			bool hasLink = std::find(node.nextAttacks.begin(), node.nextAttacks.end(), targetName) != node.nextAttacks.end();

			// どのボタンで出る攻撃かも出す（派生先は押したボタンごとに選ばれる）。
			// 表示だけ変えて、ImGui の ID は攻撃名のまま固定する
			const std::string label = std::format("{}  [{}]##{}", targetName, ButtonLabel(targetNode.condition.button), targetName);
			if (ImGui::Checkbox(label.c_str(), &hasLink)) {
				if (hasLink) {
					node.nextAttacks.push_back(targetName);
				} else {
					std::erase(node.nextAttacks, targetName);
				}
			}
		}
	}

	ImGui::EndChild();

}
#endif // _DEBUG
