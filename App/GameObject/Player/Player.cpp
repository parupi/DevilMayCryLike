#include "Player.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateMove.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Object/Renderer/PrimitiveRenderer.h>
#include <3d/Collider/AABBCollider.h>
#include <3d/Collider/CollisionManager.h>
#include <base/utility/DeltaTime.h>
#include "State/PlayerStateJump.h"
#include "State/PlayerStateAir.h"
#include <numbers>
#include <3d/Primitive/PrimitiveLineDrawer.h>
#include "State/Attack/PlayerStateAttackBase.h"


Player::Player(std::string objectNama) : Object3d(objectNama)
{
	Object3d::Initialize();

	// レンダラーの生成
	//RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerBody", "PlayerBody"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerHead", "PlayerHead"));

	//AddRenderer(RendererManager::GetInstance()->FindRender("PlayerBody"));
	AddRenderer(RendererManager::GetInstance()->FindRender("PlayerHead"));

	states_["Idle"] = std::make_unique<PlayerStateIdle>();
	states_["Move"] = std::make_unique<PlayerStateMove>();
	states_["Jump"] = std::make_unique<PlayerStateJump>();
	states_["Air"] = std::make_unique<PlayerStateAir>();
	states_["AttackComboA1"] = std::make_unique<PlayerStateAttackBase>("AttackComboA1");
	states_["AttackComboA2"] = std::make_unique<PlayerStateAttackBase>("AttackComboA2");
	states_["AttackComboA3"] = std::make_unique<PlayerStateAttackBase>("AttackComboA3");
	states_["AttackComboB2"] = std::make_unique<PlayerStateAttackBase>("AttackComboB2");
	states_["AttackComboB3"] = std::make_unique<PlayerStateAttackBase>("AttackComboB3");
	states_["AttackHighTime"] = std::make_unique<PlayerStateAttackBase>("AttackHighTime");
	states_["AttackAerialRave1"] = std::make_unique<PlayerStateAttackBase>("AttackAerialRave1");
	states_["AttackAerialRave2"] = std::make_unique<PlayerStateAttackBase>("AttackAerialRave2");
	currentState_ = states_["Idle"].get();
}

void Player::Initialize()
{
	GetCollider(name_)->category_ = CollisionCategory::Player;
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMax *= 0.5f;
	static_cast<AABBCollider*>(GetCollider(name_))->GetColliderData().offsetMin *= 0.5f;
	// 武器のレンダラー生成
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerWeapon", "Sword"));
	// 武器用のコライダー生成
	CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>("WeaponCollider"));
	// 武器を生成
	weapon_ = std::make_unique<PlayerWeapon>("PlayerWeapon");

	weapon_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerWeapon"));

	weapon_->AddCollider(CollisionManager::GetInstance()->FindCollider("WeaponCollider"));

	weapon_->Initialize();

	weapon_->GetWorldTransform()->SetParent(GetWorldTransform());

	//GetRenderer("PlayerHead")->GetWorldTransform()->GetTranslation().y -= 1.5f;

	scoreManager = std::make_unique<StylishScoreManager>();



	weapon_->SetScoreManager(scoreManager.get());

	hitStop_ = std::make_unique<HitStop>();

	titleWord_ = std::make_unique<Sprite>();
	titleWord_->Initialize("uvChecker.png");
	titleWord_->SetSize({ 32.0f, 32.0f });
	titleWord_->SetAnchorPoint({ 0.5f, 0.5f });

	// 全攻撃を一度更新しておく
	for (auto& state : states_) {
		PlayerStateAttackBase* attackState = dynamic_cast<PlayerStateAttackBase*>(state.second.get());
		if (attackState) {
			attackState->UpdateAttackData();
		}
	}
}

void Player::Update()
{
	if (currentState_) {
		currentState_->Update(*this);
	}


	hitStop_->Update();
	scoreManager->Update();

	LockOn();

	GetWorldTransform()->GetTranslation() += velocity_ * DeltaTime::GetDeltaTime();
	velocity_ += acceleration_ * DeltaTime::GetDeltaTime();

	// 毎フレーム切っておく
	onGround_ = false;

	weapon_->Update();
	Object3d::Update();

	// 全攻撃を更新
	for (auto& state : states_) {
		PlayerStateAttackBase* attackState = dynamic_cast<PlayerStateAttackBase*>(state.second.get());
		if (attackState) {
			attackState->UpdateAttackData();
		}
	}

#ifdef _DEBUG
	// エディターの描画
	DrawAttackDataEditorUI();
#endif // DEBUG

	if (lockOnEnemy_) {
		titleWord_->SetPosition(CameraManager::GetInstance()->GetActiveCamera()->WorldToScreen(lockOnEnemy_->GetWorldTransform()->GetTranslation(), 1280, 720));
		titleWord_->Update();
	}
}

void Player::Draw()
{
	weapon_->Draw();
	Object3d::Draw();

	for (auto& [name, state] : states_) {
		PlayerStateAttackBase* attackState = dynamic_cast<PlayerStateAttackBase*>(state.get());
		if (attackState) {
			attackState->DrawControlPoints(*this);
		}
	}
}

void Player::DrawEffect()
{
	if (isLockOn_) {
		titleWord_->Draw();
	}
}

void Player::DrawAttackDataEditor(PlayerStateAttackBase* attack)
{
	const char* attackName = attack->name_.c_str();

	int32_t& pointCount = gv->GetValueRef<int32_t>(attackName, "PointCount");

	for (int32_t i = 0; i < pointCount; ++i) {
		Vector3& point = gv->GetValueRef<Vector3>(attackName, "ControlPoint_" + std::to_string(i));
		ImGui::DragFloat3(("P" + std::to_string(i)).c_str(), &point.x, 0.01f);
	}

	if (ImGui::Button("Add Point")) {
		gv->AddItem(attackName, "ControlPoint_" + std::to_string(pointCount), Vector3{});
		++pointCount;
	}

	if (ImGui::Button("Remove Last") && pointCount > 0) {
		--pointCount;
		gv->RemoveItem(attackName, "ControlPoint_" + std::to_string(pointCount));
	}
	ImGui::Separator();

	// 移動系
	ImGui::DragFloat3("Move Speed", &gv->GetValueRef<Vector3>(attackName, "MoveSpeed").x, 0.01f);
	ImGui::DragFloat3("KnockBack Speed", &gv->GetValueRef<Vector3>(attackName, "KnockBackSpeed").x, 0.01f);

	ImGui::Separator();

	// タイマー系
	ImGui::DragFloat("Total Duration", &gv->GetValueRef<float>(attackName, "TotalDuration"), 0.01f);
	ImGui::DragFloat("Pre Delay", &gv->GetValueRef<float>(attackName, "PreDelay"), 0.01f);
	ImGui::DragFloat("Attack Duration", &gv->GetValueRef<float>(attackName, "AttackDuration"), 0.01f);
	ImGui::DragFloat("Post Delay", &gv->GetValueRef<float>(attackName, "PostDelay"), 0.01f);
	ImGui::DragFloat("Next Attack Delay", &gv->GetValueRef<float>(attackName, "NextAttackDelay"), 0.01f);

	ImGui::Separator();

	// その他
	ImGui::Checkbox("Draw Debug Control Points", &gv->GetValueRef<bool>(attackName, "DrawDebugControlPoints"));

	ImGui::DragFloat("Damage", &gv->GetValueRef<float>(attackName, "Damage"), 0.01f);

	ImGui::DragFloat("HitStopTime", &gv->GetValueRef<float>(attackName, "HitStopTime"), 0.01f);
	ImGui::DragFloat("HitStopIntensity", &gv->GetValueRef<float>(attackName, "HitStopIntensity"), 0.01f);

	// 攻撃時に地上にいるかの判定
	ImGui::Separator();

	ImGui::Text("Posture:");
	ImGui::SameLine();
	ImGui::RadioButton("Stand", &gv->GetValueRef<int32_t>(attackName, "AttackPosture"), 0);
	ImGui::SameLine();
	ImGui::RadioButton("Air", &gv->GetValueRef<int32_t>(attackName, "AttackPosture"), 1);

	// === 派生攻撃インデックス ===
	ImGui::Separator();
	ImGui::Text("Next Attacks");

	int32_t& nextAttackCount = gv->GetValueRef<int32_t>(attack->name_, "NextAttackCount");
	ImGui::DragInt("Next Attack Count", &nextAttackCount, 1, 0, 5);
	nextAttackCount = std::clamp(nextAttackCount, 0, 3);

	// 全攻撃名リストの取得
	std::vector<std::string> attackNames;
	for (auto& [name, state] : states_) {
		if (dynamic_cast<PlayerStateAttackBase*>(state.get())) {
			attackNames.push_back(name);
		}
	}
	std::vector<const char*> cstrs;
	for (const auto& name : attackNames) {
		cstrs.push_back(name.c_str());
	}

	// 複数の派生攻撃を選択
	for (int i = 0; i < nextAttackCount; ++i) {
		std::string key = "NextAttackIndex_" + std::to_string(i);
		int32_t& index = gv->GetValueRef<int32_t>(attack->name_, key);
		if (index < 0 || index >= (int32_t)attackNames.size()) {
			index = 0; // 範囲外防止
		}
		std::string label = "Next Attack " + std::to_string(i);
		ImGui::Combo(label.c_str(), &index, cstrs.data(), static_cast<int>(cstrs.size()));
	}

	ImGui::Separator();

	if (ImGui::Button("Save")) {
		gv->SaveFile(attackName);
		std::string message = std::format("{}.json saved.", attackName);
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
	}

}

void Player::DrawAttackDataEditorUI()
{
	// 攻撃ステートを収集
	std::vector<PlayerStateAttackBase*> attackStates;
	std::vector<std::string> attackNames;
	for (auto& [name, state] : states_) {
		if (auto* attack = dynamic_cast<PlayerStateAttackBase*>(state.get())) {
			attackStates.push_back(attack);
			attackNames.push_back(attack->name_);
		}
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

	ImGui::Begin("Attack Editor");

	if (ImGui::Combo("Select Attack", &currentIndex, nameCStrs.data(), static_cast<int>(nameCStrs.size()))) {
		// 選択が変わったら必要に応じて処理
	}

	// 選択中の攻撃ステートのエディタを表示
	PlayerStateAttackBase* selectedAttack = attackStates[currentIndex];
	if (selectedAttack) {
		DrawAttackDataEditor(selectedAttack);
	}

	ImGui::End();
}

#ifdef _DEBUG
void Player::DebugGui()
{
	// 現在のステート名を取得
	const char* currentStateName = "Unknown";
	for (const auto& [named, state] : states_) {
		if (state.get() == currentState_) {
			currentStateName = named.c_str();
			break;
		}
	}

	ImGui::Begin("Player");
	ImGui::Text("Current State: %s", currentStateName);
	Object3d::DebugGui();
	ImGui::End();

	ImGui::Begin("Weapon");
	weapon_->DebugGui();
	ImGui::End();

}

#endif // _DEBUG

void Player::ChangeState(const std::string& stateName)
{
	currentState_->Exit(*this);
	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();
		currentState_->Enter(*this);
	}
}

void Player::Move()
{
	input = Input::GetInstance();
	Camera* camera = CameraManager::GetInstance()->GetActiveCamera();
	if (!camera) return;

	// 各フレームでまず速度をゼロに初期化
	velocity_ = { 0.0f, velocity_.y, 0.0f };
	// 入力方向をローカル（プレイヤーから見た）方向で作成
	Vector3 inputDir = { 0.0f, 0.0f, 0.0f };

	if (input->IsConnected()) {
		if (input->GetLeftStickY() != 0.0f) {
			inputDir.z = input->GetLeftStickY();
		}
		if (input->GetLeftStickX() != 0.0f) {
			inputDir.x = input->GetLeftStickX();
		}
	} else {
		if (Input::GetInstance()->PushKey(DIK_W)) inputDir.z += 1.0f;
		if (Input::GetInstance()->PushKey(DIK_S)) inputDir.z -= 1.0f;
		if (Input::GetInstance()->PushKey(DIK_D)) inputDir.x += 1.0f;
		if (Input::GetInstance()->PushKey(DIK_A)) inputDir.x -= 1.0f;
	}

	if (Length(inputDir) > 0.01f) {
		inputDir = Normalize(inputDir);

		// カメラのforward/right（Y方向カット）
		Vector3 camForward = camera->GetForward(); // カメラの「向き」
		camForward.y = 0.0f;
		camForward = Normalize(camForward);

		Vector3 camRight = camera->GetRight(); // カメラの右
		camRight.y = 0.0f;
		camRight = Normalize(camRight);

		// 入力方向をカメラの向きに投影
		Vector3 moveDir = camRight * inputDir.x + camForward * inputDir.z;
		moveDir = Normalize(moveDir);

		// 移動速度に反映
		float velocityY = velocity_.y;
		velocity_ = moveDir * 10.0f;
		velocity_.y = velocityY;

		// ロックオンしていなければプレイヤーの向きも更新
		if (!isLockOn_) {
			if (Length(moveDir) > 0.001f) {
				moveDir.x *= -1.0f;
				Quaternion lookRot = LookRotation(moveDir);

				GetWorldTransform()->GetRotation() = lookRot;
			}
		}
	}
}

void Player::LockOn()
{
	enemies_.clear();
	std::vector<Object3d*> objects;
	objects = Object3dManager::GetInstance()->GetAllObject();
	for (auto& object : objects) {
		if (!object->name_.find("HellKaina")) {
			enemies_.push_back(static_cast<Enemy*>(object));
		}
	}

	if (input->TriggerButton(PadNumber::ButtonR) || input->TriggerKey(DIK_P)) {
		isLockOn_ = true;
		// 敵を設定
		float lowDistance = 300.0f;
		for (auto& enemy : enemies_) {

			float length = Length(enemy->GetWorldTransform()->GetTranslation() - GetWorldTransform()->GetTranslation());
			if (length < lowDistance) {
				lowDistance = length;
				lockOnEnemy_ = enemy;
			}
		}
	}

	if (!input->PushButton(PadNumber::ButtonR) && !input->PushKey(DIK_P)) {
		isLockOn_ = false;
	}

	if (isLockOn_) {
		if (lockOnEnemy_->IsAlive()) {
			Vector3 direction = Normalize(lockOnEnemy_->GetWorldTransform()->GetTranslation() - GetWorldTransform()->GetTranslation());
			if (Length(direction) > 0.001f) {
				direction.x *= -1.0f;
				Quaternion lookRot = LookRotation(direction);

				GetWorldTransform()->GetRotation() = lookRot;
			}
		} else {
			isLockOn_ = false;
			lockOnEnemy_ = nullptr;
		}
	}
}

std::string Player::GetAttackStateNameByIndex(int32_t index) const
{
	int i = 0;
	for (const auto& [name, state] : states_) {
		if (dynamic_cast<PlayerStateAttackBase*>(state.get())) {
			if (i == index) return name;
			++i;
		}
	}
	return "";
}

int32_t Player::GetAttackStateCount() const
{
	int32_t count = 0;
	for (const auto& [_, state] : states_) {
		if (dynamic_cast<PlayerStateAttackBase*>(state.get())) {
			++count;
		}
	}
	return count;
}

void Player::OnCollisionEnter(BaseCollider* other)
{
	if (other->category_ == CollisionCategory::Ground) {

		AABBCollider* playerCollider = static_cast<AABBCollider*>(GetCollider("Player"));

		AABBCollider* blockCollider = static_cast<AABBCollider*>(other);

		Vector3 outNormal = AABBCollider::CalculateCollisionNormal(playerCollider, blockCollider);

		Vector3 playerMin = playerCollider->GetMin();
		Vector3 playerMax = playerCollider->GetMax();

		float playerOffset = (playerMax.x - playerMin.x) * 0.5f;

		if (outNormal.x == 1.0f) {
			// 右に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + playerOffset;
		} else if (outNormal.x == -1.0f) {
			// 左に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - playerOffset;
		} else if (outNormal.y == 1.0f) {
			// 上に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + playerOffset;
			GetWorldTransform()->GetTranslation().y -= 0.1f;
			//velocity_.y = 0.0f;
			onGround_ = true;
		} else if (outNormal.y == -1.0f) {
			// 下に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - playerOffset;
			//velocity_ *= -1.0f;
		} else if (outNormal.z == 1.0f) {
			// 奥に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + playerOffset;
		} else if (outNormal.z == -1.0f) {
			// 手前に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - playerOffset;
		}
	}
}

void Player::OnCollisionStay(BaseCollider* other)
{
	if (other->category_ == CollisionCategory::Ground) {

		AABBCollider* playerCollider = static_cast<AABBCollider*>(GetCollider("Player"));

		AABBCollider* blockCollider = static_cast<AABBCollider*>(other);

		Vector3 outNormal = AABBCollider::CalculateCollisionNormal(playerCollider, blockCollider);

		Vector3 playerMin = playerCollider->GetMin();
		Vector3 playerMax = playerCollider->GetMax();

		float playerOffset = (playerMax.x - playerMin.x) * 0.5f;

		if (outNormal.x == 1.0f) {
			// 右に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMax().x + playerOffset;
		} else if (outNormal.x == -1.0f) {
			// 左に当たってる
			GetWorldTransform()->GetTranslation().x = blockCollider->GetMin().x - playerOffset;
		} else if (outNormal.y == 1.0f) {
			// 上に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMax().y + playerOffset;
			GetWorldTransform()->GetTranslation().y -= 0.1f;
			//velocity_.y = 0.0f;
			onGround_ = true;
		} else if (outNormal.y == -1.0f) {
			// 下に当たってる
			GetWorldTransform()->GetTranslation().y = blockCollider->GetMin().y - playerOffset;
			//velocity_ *= -1.0f;
		} else if (outNormal.z == 1.0f) {
			// 奥に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMax().z + playerOffset;
		} else if (outNormal.z == -1.0f) {
			// 手前に当たってる
			GetWorldTransform()->GetTranslation().z = blockCollider->GetMin().z - playerOffset;
		}
	}
}

void Player::OnCollisionExit(BaseCollider* other)
{
	other;
}
