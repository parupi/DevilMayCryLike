#include "PlayerStateAttack.h"
#include "Debugger/GlobalVariables.h"
#include "World3D/Primitive/PrimitiveLineDrawer.h"
#include <Math/MathUtils.h>
#include "GameObject/Character/Player/Player.h"
#include "World3D/Collider/AABBCollider.h"
#include "Utility/DeltaTime.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include "Audio/SoundManager.h"
#ifdef _DEBUG
#include "Editor/Core/EditorDebugDraw.h"
#endif

#include <vector>

namespace {
	// 攻撃に割り当てたボタンで出せるか（ボタン指定なしの攻撃はどのボタンでもよい）
	bool AcceptsButton(const AttackNode& node, InputButton button) {
		return node.condition.button == InputButton::None || node.condition.button == button;
	}
}

PlayerStateAttack::PlayerStateAttack(std::string attackName) {
	name_ = attackName;
	std::string groupName = "Attack/" + name_;
	// 攻撃のデータを生成
	gv->CreateGroup(groupName);

	gv->AddItem(name_, "PointCount", int32_t()); // 制御点の数
	// 移動系
	gv->AddItem(name_, "MoveSpeed", Vector3());          // 攻撃中にどれくらい移動するか

	// タイマー系
	gv->AddItem(name_, "TotalDuration", float());      // 攻撃全体にかかる時間
	gv->AddItem(name_, "PreDelay", float());           // 予備動作の時間
	gv->AddItem(name_, "AttackDuration", float());     // 攻撃にかかる時間
	gv->AddItem(name_, "PostDelay", float());          // 技後の硬直時間
	gv->AddItem(name_, "NextAttackDelay", float());    // 次の攻撃が出せるまでの時間

	// その他
	gv->AddItem(name_, "DrawDebugControlPoints", bool()); // 制御点のデバッグ描画フラグ
	gv->AddItem(name_, "IsMove", bool(false)); // 攻撃中に移動するかどうか(モーション用のフラグ)

	// ダメージなどの汎用パラメータ
	gv->AddItem(name_, "Damage", float());

	// 派生先インデックスの個数
	gv->AddItem(name_, "NextAttackCount", int32_t(0));

	// 初期で最大3個まで確保（必要なら動的にも増やせる）
	for (int i = 0; i < 3; ++i) {
		gv->AddItem(name_, "NextAttack_" + std::to_string(i), std::string("")); // -1 = 無効
	}

	gv->AddItem(name_, "AttackPosture", int32_t(0));

	gv->AddItem(name_, "HitStopTime", float());

	gv->AddItem(name_, "HitStopIntensity", float());

	// 攻撃の強さ（0=Light, 1=Medium, 2=Heavy）。未設定の攻撃は従来通り完全停止のHeavy
	gv->AddItem(name_, "HitStopStrength", int32_t(static_cast<int32_t>(HitStopStrength::Heavy)));

	// ── 攻撃を受けた側に送るノックバック情報（仕様書 §3）──
	// 既定値はどれも、作り直す前と同じ挙動になる値にしてある
	gv->AddItem(name_, "ReactionType", int32_t(0));
	// ノックバック＆打ち上げ共通
	gv->AddItem(name_, "ImpulseForce", float());
	gv->AddItem(name_, "UpwardRatio", float());
	// 吹っ飛び用
	gv->AddItem(name_, "TorqueForce", float());
	// のけぞり用
	gv->AddItem(name_, "StunTime", float());
	// ノックバックの時間と減速（仕様書 §7）。0 なら ReactionType ごとの既定値
	gv->AddItem(name_, "KnockbackDuration", float());
	gv->AddItem(name_, "KnockbackDeceleration", 1.0f);
	gv->AddItem(name_, "KnockbackMaxSpeed", float());
	// 向き（仕様書 §4）と、連続ヒット時の合成方法（§8）
	gv->AddItem(name_, "KnockbackDirection", int32_t(static_cast<int32_t>(KnockbackDirection::AwayFromAttacker)));
	gv->AddItem(name_, "KnockbackBlend", int32_t(static_cast<int32_t>(KnockbackBlend::Override)));
	gv->AddItem(name_, "KnockbackOverrideVelocity", bool(true));
	gv->AddItem(name_, "KnockbackCanLaunch", bool(true));

	gv->AddItem(name_, "ButtonIndex", int32_t(0));
	gv->AddItem(name_, "LockOnFlag", bool(false));
	gv->AddItem(name_, "RootAttackFlag", bool(false));
	gv->AddItem(name_, "IsAir", bool(false));
	gv->AddItem(name_, "DirIndex", int32_t(0));

	// ── ここから強攻撃用。既定値はどれも、今までの攻撃の挙動を変えない値にしてある ──

	// ジャスト回避の直後（カウンター受付中）だけ出せる攻撃にするか
	gv->AddItem(name_, "CounterFlag", bool(false));

	// 多段ヒット・当たり判定
	gv->AddItem(name_, "HitCount", int32_t(1));          // 攻撃判定が出ている間に何回当たるか
	gv->AddItem(name_, "HitboxScale", 1.0f);             // 武器の当たり判定の大きさの倍率
	// 最終段だけ別の性能にする（HitCount が2以上のときだけ使う）
	gv->AddItem(name_, "UseFinalHit", bool(false));
	gv->AddItem(name_, "FinalDamage", float());
	gv->AddItem(name_, "FinalReactionType", int32_t(static_cast<int32_t>(ReactionType::Knockback)));
	gv->AddItem(name_, "FinalImpulseForce", float());
	gv->AddItem(name_, "FinalUpwardRatio", float());
	gv->AddItem(name_, "FinalTorqueForce", float());
	gv->AddItem(name_, "FinalStunTime", float());
	gv->AddItem(name_, "FinalHitStopTime", float());

	// 溜め
	gv->AddItem(name_, "IsCharge", bool(false));
	gv->AddItem(name_, "ChargeMinTime", 0.2f);           // これより短く離すと溜め無し
	gv->AddItem(name_, "ChargeMaxTime", 1.0f);           // ここで最大
	gv->AddItem(name_, "ChargeDamageScale", 1.0f);       // 最大まで溜めたときのダメージ倍率
	gv->AddItem(name_, "ChargeImpulseScale", 1.0f);      // 同・吹き飛ばしの強さの倍率
	gv->AddItem(name_, "ChargeHitStopScale", 1.0f);      // 同・ヒットストップの長さの倍率

	// 攻撃の出始めから被弾しない時間
	gv->AddItem(name_, "InvincibleTime", float());

	// 見た目の種類（AttackVfxStyle）。0 = Auto は攻撃の性能から決める
	gv->AddItem(name_, "VfxStyle", int32_t(0));
}

void PlayerStateAttack::Enter(Player& player) {
	attackPhase_ = AttackPhase::Startup;
	stateTime_.current = 0.0f;
	stateTime_.max = attackData_.totalDuration;

	attackData_.pointCount = gv->GetValueRef<int32_t>(name_, "PointCount");

	attackChangeTimer_.max = attackData_.nextAttackDelay;
	attackChangeTimer_.current = 0.0f;

	for (int32_t i = 0; i < attackData_.pointCount; i++) {
		attackData_.controlPoints.push_back(gv->GetValueRef<Vector3>(name_, "ControlPoint_" + std::to_string(i)));
		attackData_.controlRotations.push_back(gv->GetValueRef<Vector3>(name_, "ControlRotation_" + std::to_string(i)));
	}

	// 溜めと多段ヒットの進み具合を戻す
	chargeTime_ = 0.0f;
	chargeRatio_ = 0.0f;
	isChargeReleased_ = false;
	isChargeFullNotified_ = false;
	hitIndex_ = 0;

	// 今回の攻撃のパラメータを送っておく（最終段・溜めの差し替えもここを通す）
	ApplyHitData(player);

	// 攻撃の出始めの無敵
	if (attackData_.invincibleTime > 0.0f) {
		player.GrantAttackInvincibility(attackData_.invincibleTime);
	}

	// 振り始めに剣風の音を鳴らす。
	// 納刀モーションも同じ仕組みで流れてくるが、あれは攻撃ではないので鳴らさない。
	// 溜め攻撃は構えで止まるので、振り始める ReleaseCharge で鳴らす
	if (name_ != "Sheathe" && !attackData_.isCharge) {
		SoundManager::GetInstance().PlaySE("SwordSlash", 0.55f);
	}

	isFinish_ = false;
}

void PlayerStateAttack::Update(Player& player, float deltaTime) {
	// 溜め攻撃は、構えきってからボタンを離すまで攻撃の時間を進めない
	if (UpdateCharge(player, deltaTime)) {
		return;
	}

	stateTime_.current += deltaTime;

	// 攻撃フェーズの更新処理
	AttackPhase prevPhase = attackPhase_;
	UpdatePhase(stateTime_.current);

	// Cancel フェーズに入った瞬間に先行入力を発火
	if (prevPhase != AttackPhase::Cancel && attackPhase_ == AttackPhase::Cancel && hasPendingBuffer_) {
		pendingRequest_ = BuildRequestFromNode(player, pendingButton_);
		hasPendingBuffer_ = false;
	}

	// フェーズごとの処理を追加
	switch (attackPhase_) {
	case AttackPhase::Startup:
	{
		UpdateStartup(player, deltaTime);
		break;
	}
	case AttackPhase::Charge:
		// 溜め中は UpdateCharge が受け持つので、ここへは来ない
		break;
	case AttackPhase::Active:
	{
		UpdateActive(player);
		UpdateHitSegment(player);
		break;
	}
	case AttackPhase::Recovery:
		UpdateRecovery(player);
		break;
	case AttackPhase::Cancel:
		attackChangeTimer_.current += deltaTime;

		// 状態遷移
		if (stateTime_.current >= attackData_.totalDuration) {
			isFinish_ = true;
		}

		break;
	}

	// 移動可能の場合は移動させる
	if (attackData_.isMove) {
		player.Move(player.GetMoveDirection(), deltaTime);
	}
}

void PlayerStateAttack::Exit(Player& player) {
	player;
	attackPhase_ = AttackPhase::Startup;
	stateTime_.current = 0.0f;
	stateTime_.max = attackData_.totalDuration;

	attackChangeTimer_.current = 0.0f;
	hasPendingBuffer_ = false;
	pendingRequest_ = {};

	// 溜めの途中で中断された（回避・被弾）ときに、溜めた状態を次へ持ち越さない
	chargeTime_ = 0.0f;
	chargeRatio_ = 0.0f;
	isChargeReleased_ = false;
	isChargeFullNotified_ = false;
	hitIndex_ = 0;
}

AttackRequestData PlayerStateAttack::BuildRequestFromNode(Player& player, InputButton button) {
	AttackRequestData req{};
	req.type = AttackRequest::None;

	const PlayerCombat* combat = player.GetCombat();
	const AttackNode& node = combat->GetAttackNode(name_);

	// 押したボタンで出せる派生先だけを候補にして、その中から入力のタイミングで選ぶ。
	// 以前は全派生先をタイミングだけで選んでからボタンで弾いていたので、
	// 「Y の派生と X の派生を両方持つ攻撃」を作っても、タイミング次第で X が出なかった
	std::vector<const std::string*> candidates;
	for (const std::string& nextAttack : node.nextAttacks) {
		if (AcceptsButton(combat->GetAttackNode(nextAttack), button)) {
			candidates.push_back(&nextAttack);
		}
	}

	const int candidateCount = static_cast<int>(candidates.size());
	if (candidateCount > 0 && attackChangeTimer_.max > 0.0f) {
		float segment = attackChangeTimer_.max / static_cast<float>(candidateCount);
		int derivedIndex = std::clamp(static_cast<int>(attackChangeTimer_.current / segment), 0, candidateCount - 1);
		req.nextAttack = *candidates[derivedIndex];
		req.type = AttackRequest::ChangeAttack;
	}

	return req;
}

bool PlayerStateAttack::CanBranchWith(Player& player, InputButton button) const {
	const PlayerCombat* combat = player.GetCombat();
	const AttackNode& node = combat->GetAttackNode(name_);

	for (const std::string& nextAttack : node.nextAttacks) {
		if (AcceptsButton(combat->GetAttackNode(nextAttack), button)) {
			return true;
		}
	}
	return false;
}

AttackRequestData PlayerStateAttack::ExecuteCommand(Player& player, const PlayerCommand& command) {
	AttackRequestData req{};
	req.nextAttack = "";
	req.type = AttackRequest::None;

	if (command.action == PlayerAction::Attack) {
		// どの派生先にも割り当てていないボタンは無視する。
		// 先行入力に積むと、先に押した正しいボタンの入力を上書きして消してしまうため
		if (!CanBranchWith(player, command.button)) {
			return req;
		}
		if (attackPhase_ != AttackPhase::Cancel) {
			// Cancel フェーズ前の入力をバッファに保存（上書きで最新入力を保持）
			hasPendingBuffer_ = true;
			pendingButton_ = command.button;
			return req;
		}
		return BuildRequestFromNode(player, command.button);
	}
	else if (command.action == PlayerAction::Jump) {
		if (attackPhase_ != AttackPhase::Cancel || gv->GetValueRef<int32_t>(name_, "AttackPosture") == 1) {
			return req;
		}
		req.type = AttackRequest::Jump;
		return req;
	}

	return req;
}

void PlayerStateAttack::OnInterrupted(Player&) {
	// 割り込みされたので攻撃を終了させる
	isFinish_ = true;
}

KnockbackData PlayerStateAttack::LoadKnockback(const std::string& prefix) const {
	KnockbackData knockback;
	knockback.type = static_cast<ReactionType>(gv->GetValueRef<int32_t>(name_, prefix + "ReactionType"));
	knockback.power = gv->GetValueRef<float>(name_, prefix + "ImpulseForce");
	// UpwardRatio は「水平の強さに対する上方向の割合」。KnockbackData は速度で持つのでここで掛ける
	knockback.verticalPower = knockback.power * gv->GetValueRef<float>(name_, prefix + "UpwardRatio");
	knockback.torque = gv->GetValueRef<float>(name_, prefix + "TorqueForce");
	knockback.stunTime = gv->GetValueRef<float>(name_, prefix + "StunTime");

	// 以下は最終段でも同じものを使う（段ごとに変えたくなるものではないため）
	knockback.duration = gv->GetValueRef<float>(name_, "KnockbackDuration");
	knockback.deceleration = gv->GetValueRef<float>(name_, "KnockbackDeceleration");
	knockback.maxSpeed = gv->GetValueRef<float>(name_, "KnockbackMaxSpeed");
	knockback.direction = static_cast<KnockbackDirection>(gv->GetValueRef<int32_t>(name_, "KnockbackDirection"));
	knockback.blend = static_cast<KnockbackBlend>(gv->GetValueRef<int32_t>(name_, "KnockbackBlend"));
	knockback.overrideVelocity = gv->GetValueRef<bool>(name_, "KnockbackOverrideVelocity");
	knockback.canLaunch = gv->GetValueRef<bool>(name_, "KnockbackCanLaunch");
	return knockback;
}

void PlayerStateAttack::UpdateAttackData() {
	// 制御点
	attackData_.pointCount = GlobalVariables::GetInstance().GetValueRef<int32_t>(name_, "PointCount");
	attackData_.controlPoints.resize(attackData_.pointCount);
	attackData_.controlRotations.resize(attackData_.pointCount);
	for (int32_t i = 0; i < attackData_.pointCount; i++) {
		attackData_.controlPoints[i] = GlobalVariables::GetInstance().GetValueRef<Vector3>(name_, "ControlPoint_" + std::to_string(i));
		attackData_.controlRotations[i] = GlobalVariables::GetInstance().GetValueRef<Vector3>(name_, "ControlRotation_" + std::to_string(i));
	}

	// 移動系
	attackData_.moveVelocity = GlobalVariables::GetInstance().GetValueRef<Vector3>(name_, "MoveSpeed");

	// タイマー系
	attackData_.totalDuration = GlobalVariables::GetInstance().GetValueRef<float>(name_, "TotalDuration");
	attackData_.preDelay = GlobalVariables::GetInstance().GetValueRef<float>(name_, "PreDelay");
	attackData_.attackDuration = GlobalVariables::GetInstance().GetValueRef<float>(name_, "AttackDuration");
	attackData_.postDelay = GlobalVariables::GetInstance().GetValueRef<float>(name_, "PostDelay");
	attackData_.nextAttackDelay = GlobalVariables::GetInstance().GetValueRef<float>(name_, "NextAttackDelay");

	// その他
	attackData_.isMove = GlobalVariables::GetInstance().GetValueRef<bool>(name_, "IsMove");
	attackData_.drawDebugControlPoints = GlobalVariables::GetInstance().GetValueRef<bool>(name_, "DrawDebugControlPoints");
	attackData_.damage = GlobalVariables::GetInstance().GetValueRef<float>(name_, "Damage");

	attackData_.hitStopTime = GlobalVariables::GetInstance().GetValueRef<float>(name_, "HitStopTime");

	attackData_.hitStopIntensity = GlobalVariables::GetInstance().GetValueRef<float>(name_, "HitStopIntensity");

	attackData_.hitStopStrength = HitStop::ToStrength(GlobalVariables::GetInstance().GetValueRef<int32_t>(name_, "HitStopStrength"));

	// ── 攻撃を受けた側に送るノックバック情報（仕様書 §3）──
	// エディタの調整値は ImpulseForce（水平の強さ）と UpwardRatio（それに対する上方向の割合）のまま。
	// KnockbackData は速度[m/s]で持つので、ここで掛けて verticalPower にする
	attackData_.knockback = LoadKnockback("");

	// 多段ヒット・当たり判定
	attackData_.hitCount = gv->GetValueRef<int32_t>(name_, "HitCount");
	attackData_.hitboxScale = gv->GetValueRef<float>(name_, "HitboxScale");
	attackData_.useFinalHit = gv->GetValueRef<bool>(name_, "UseFinalHit");
	attackData_.finalDamage = gv->GetValueRef<float>(name_, "FinalDamage");
	attackData_.finalHitStopTime = gv->GetValueRef<float>(name_, "FinalHitStopTime");
	attackData_.finalKnockback = LoadKnockback("Final");

	// 溜め
	attackData_.isCharge = gv->GetValueRef<bool>(name_, "IsCharge");
	attackData_.chargeMinTime = gv->GetValueRef<float>(name_, "ChargeMinTime");
	attackData_.chargeMaxTime = gv->GetValueRef<float>(name_, "ChargeMaxTime");
	attackData_.chargeDamageScale = gv->GetValueRef<float>(name_, "ChargeDamageScale");
	attackData_.chargeImpulseScale = gv->GetValueRef<float>(name_, "ChargeImpulseScale");
	attackData_.chargeHitStopScale = gv->GetValueRef<float>(name_, "ChargeHitStopScale");

	// 無敵
	attackData_.invincibleTime = gv->GetValueRef<float>(name_, "InvincibleTime");

	// 見た目の種類（範囲外の値は Auto に落とさず端へ寄せる）
	attackData_.vfxStyle = static_cast<AttackVfxStyle>(std::clamp(gv->GetValueRef<int32_t>(name_, "VfxStyle"),
		0, static_cast<int32_t>(AttackVfxStyle::Count) - 1));
}

void PlayerStateAttack::DrawControlPoints(Player& player) {
	if (attackData_.pointCount < 4 || !attackData_.drawDebugControlPoints) return;
#ifdef _DEBUG
	// 攻撃ごとのフラグに加えて、エディタのDebug Drawメニューでも一括で消せるようにする
	if (!EditorDebugDraw::IsEnabled(EditorDebugDraw::Flag::AttackTrail)) return;
#endif

	// 制御点の位置に球を描画
	for (int32_t i = 0; i < attackData_.pointCount; ++i) {
		PrimitiveLineDrawer::GetInstance().DrawWireSphere(player.GetWorldTransform()->GetTranslation() + attackData_.controlPoints[i], 0.05f, { 1, 1, 1, 1 }, 24);
	}

	// 曲線がどんな感じになるか計算
	std::vector<Vector3> curvePoints;
	if (attackData_.pointCount >= 4) {
		const float step = 0.1f;
		for (float t = 0.0f; t <= 1.0f; t += step) {
			Vector3 point = CatmullRomSpline(attackData_.controlPoints, t);
			curvePoints.push_back(point);
		}
	}
	// 計算した曲線を描画
	for (size_t i = 0; i < curvePoints.size() - 1; i++) {
		PrimitiveLineDrawer::GetInstance().DrawLine(player.GetWorldTransform()->GetTranslation() + curvePoints[i], player.GetWorldTransform()->GetTranslation() + curvePoints[i + 1], { 1.0f, 1.0f, 1.0f, 1.0f });
	}
}

bool PlayerStateAttack::HasBranch(Player& player) const {
	const AttackNode& node = player.GetCombat()->GetAttackNode(name_);

	int nextCount = static_cast<int>(node.nextAttacks.size());

	return nextCount > 0;
}

bool PlayerStateAttack::UpdateCharge(Player& player, float deltaTime) {
	if (!attackData_.isCharge || isChargeReleased_) return false;

	const bool isHeld = IsAttackButtonHeld(player);

	if (attackPhase_ != AttackPhase::Charge) {
		// 構えの途中で離したら、溜めずにそのまま振る（軽く押しただけでも攻撃として出す）
		if (!isHeld) {
			ReleaseCharge(player);
			return false;
		}
		// まだ構えの途中。予備動作は通常どおり進める
		if (stateTime_.current + deltaTime < attackData_.preDelay) {
			return false;
		}
		// 構えきったので溜めに入る
		stateTime_.current = attackData_.preDelay;
		attackPhase_ = AttackPhase::Charge;
	}

	chargeTime_ += deltaTime;

	// 溜めきった瞬間を1回だけ知らせる
	if (!isChargeFullNotified_ && chargeTime_ >= attackData_.chargeMaxTime) {
		isChargeFullNotified_ = true;
		if (CharacterLight* light = player.GetCharacterLight()) {
			light->Flash();
		}
	}

	// 構えたまま止まる。向きだけはスティックで変えられる（ロックオン中は敵を向いたまま）
	player.Rotate(player.GetMoveDirection(), deltaTime);
	Vector3& velocity = player.GetVelocity();
	velocity.x = 0.0f;
	velocity.z = 0.0f;
	if (!attackData_.controlPoints.empty()) {
		player.GetWeapon()->GetWorldTransform()->GetTranslation() = attackData_.controlPoints[0];
	}

	// 離したフレームから振り始める
	if (!isHeld) {
		ReleaseCharge(player);
		return false;
	}
	return true;
}

void PlayerStateAttack::ReleaseCharge(Player& player) {
	isChargeReleased_ = true;

	// Charge Min Time までは倍率 1.0、Charge Max Time で最大。その間は線形に上がる
	const float range = attackData_.chargeMaxTime - attackData_.chargeMinTime;
	if (range > 0.0f) {
		chargeRatio_ = std::clamp((chargeTime_ - attackData_.chargeMinTime) / range, 0.0f, 1.0f);
	} else {
		chargeRatio_ = (chargeTime_ >= attackData_.chargeMaxTime) ? 1.0f : 0.0f;
	}
	ApplyHitData(player);

	// 溜め攻撃の剣風は、構えた瞬間ではなく振り始めに鳴らす
	SoundManager::GetInstance().PlaySE("SwordSlash", 0.55f);
}

bool PlayerStateAttack::IsAttackButtonHeld(Player& player) const {
	PlayerInput* input = player.GetInput();
	if (!input) return false;
	// この攻撃に割り当てたボタンで見る（ボタン指定なしならどちらのボタンでもよい）
	return input->IsAttackButtonHeld(player.GetCombat()->GetAttackNode(name_).condition.button);
}

void PlayerStateAttack::UpdateHitSegment(Player& player) {
	const int32_t hitCount = (std::max)(attackData_.hitCount, 1);
	if (hitCount <= 1) return;

	// Attack Duration を Hit Count 等分して、今が何段目かを出す
	const float t = (attackData_.attackDuration > 0.0f)
		? std::clamp((stateTime_.current - attackData_.preDelay) / attackData_.attackDuration, 0.0f, 1.0f)
		: 1.0f;
	const int32_t index = (std::min)(static_cast<int32_t>(t * static_cast<float>(hitCount)), hitCount - 1);
	if (index == hitIndex_) return;
	hitIndex_ = index;

	// 触れたままの敵にも次の段が当たるよう、武器の判定を1回だけ切る
	player.GetWeapon()->RequestRehit();
	// 最終段だけ性能を差し替える場合に備えて、攻撃データを渡し直す
	ApplyHitData(player);
}

void PlayerStateAttack::ApplyHitData(Player& player) {
	AttackData data = attackData_;

	// 多段ヒットの最終段
	const int32_t hitCount = (std::max)(attackData_.hitCount, 1);
	if (attackData_.useFinalHit && hitCount >= 2 && hitIndex_ >= hitCount - 1) {
		data.damage = attackData_.finalDamage;
		data.hitStopTime = attackData_.finalHitStopTime;
		data.knockback = attackData_.finalKnockback;
	}

	// 溜め具合の倍率（溜め攻撃でなければ掛けない）
	if (attackData_.isCharge) {
		const float impulseScale = 1.0f + (attackData_.chargeImpulseScale - 1.0f) * chargeRatio_;
		data.damage *= 1.0f + (attackData_.chargeDamageScale - 1.0f) * chargeRatio_;
		data.hitStopTime *= 1.0f + (attackData_.chargeHitStopScale - 1.0f) * chargeRatio_;
		// 水平と上方向を同じ倍率で伸ばす（片方だけだと溜めるほど角度が変わってしまう）
		data.knockback.power *= impulseScale;
		data.knockback.verticalPower *= impulseScale;
	}

	player.SetAttackData(data);
}

void PlayerStateAttack::UpdatePhase(float time) {
	if (time < attackData_.preDelay) {
		attackPhase_ = AttackPhase::Startup;
	}
	else if (time < attackData_.preDelay + attackData_.attackDuration) {
		attackPhase_ = AttackPhase::Active;
	}
	else if (time < attackData_.preDelay + attackData_.attackDuration + attackData_.postDelay) {
		attackPhase_ = AttackPhase::Recovery;
	}
	else {
		attackPhase_ = AttackPhase::Cancel;
	}
}

void PlayerStateAttack::UpdateStartup(Player& player, float deltaTime) {
	// 方向転換
	Vector3 moveDir = player.GetMoveDirection();
	// プレイヤーの向きを移動方向に向ける
	player.Rotate(moveDir, deltaTime);

	// 制御点がまだ無い攻撃（エディタで追加した直後など）は武器を動かさない
	if (attackData_.controlPoints.empty()) return;

	// 武器を制御点の最初の位置に移動させる
	Vector3 targetPos = attackData_.controlPoints[0];
	// 武器の初期位置
	Vector3 firstPos = player.GetWeapon()->GetWorldTransform()->GetTranslation();
	// 予備動作の時間に応じて線形補間で移動させる
	Vector3 currentPos = Lerp(firstPos, targetPos, stateTime_.current / attackData_.preDelay);
	// 武器の位置を更新
	player.GetWeapon()->GetWorldTransform()->GetTranslation() = currentPos;
}

void PlayerStateAttack::UpdateActive(Player& player) {
	// 攻撃判定ON、移動処理
	player.GetWeapon()->SetIsAttack(true);
	// ローカル速度の取得
	Vector3 localVelocity = attackData_.moveVelocity;
	// プレイヤーの回転を取得
	Quaternion rotation = player.GetWorldTransform()->GetRotation();
	// ローカル速度をワールド座標に変換
	Vector3 worldVelocity = RotateVector(localVelocity, rotation);
	// 速度を設定
	player.GetVelocity() = worldVelocity;

	// 正規化t
	float activeStart = attackData_.preDelay;
	float t = std::clamp((stateTime_.current - activeStart) / attackData_.attackDuration, 0.0f, 1.0f);

	// 現在位置を計算
	Vector3 pos = CatmullRomSpline(attackData_.controlPoints, t);
	// 現在の回転を計算
	Vector3 rot = CatmullRomSpline(attackData_.controlRotations, t);

	// 位置の設定
	player.GetWeapon()->GetWorldTransform()->GetTranslation() = pos;
	// 回転の設定
	player.GetWeapon()->GetWorldTransform()->GetRotation() = EulerDegree(rot);
}

void PlayerStateAttack::UpdateRecovery(Player& player) {
	player.GetWeapon()->SetIsAttack(false);

	player.GetVelocity() = { 0.0f, 0.0f, 0.0f };
}
