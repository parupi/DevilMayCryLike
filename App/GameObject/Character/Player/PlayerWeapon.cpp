#include "PlayerWeapon.h"
#include "World3D/Collider/OBBCollider.h"
#include "World3D/Object/Renderer/BaseRenderer.h"
#include "Debugger/GlobalVariables.h"
#include "Player.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "Scene/Transition/TransitionManager.h"
#include "Math/MathUtils.h"
#include "GameObject/Effect/HitEffectSystem.h"
#include "Audio/SoundManager.h"
#include "Audio/GameSoundLibrary.h"
#include <algorithm>
#include <cmath>

namespace {
	// 攻撃名からチュートリアルの種類を判定する（該当しない攻撃はCountを返す）
	TutorialState ResolveTutorialState(const std::string& attackName) {
		if (attackName.rfind("AttackComboA", 0) == 0) return TutorialState::AttackA;
		if (attackName.rfind("AttackComboB", 0) == 0) return TutorialState::AttackB;
		if (attackName == "AttackHighTime") return TutorialState::RoundUpAttack;
		return TutorialState::Count;
	}

	Vector3 SafeNormalize(const Vector3& v, const Vector3& fallback) {
		const float length = Length(v);
		return (length > 0.0001f) ? v * (1.0f / length) : fallback;
	}

	// 刃（base→tip の線分）の上で、敵の体の中心に一番近い点。
	// そこから少し体の中へ寄せた位置を「斬った場所」とする。
	// 以前は敵の原点（体の中心）から出していたので、どこを斬っても同じ場所から火花が出ていた
	Vector3 CalcBladeHitPosition(const Vector3& bladeBase, const Vector3& bladeTip, const Vector3& bodyCenter) {
		const Vector3 blade = bladeTip - bladeBase;
		const float lengthSq = Dot(blade, blade);
		float t = (lengthSq > 0.0001f) ? Dot(bodyCenter - bladeBase, blade) / lengthSq : 1.0f;
		t = std::clamp(t, 0.0f, 1.0f);
		const Vector3 onBlade = bladeBase + blade * t;
		return onBlade + (bodyCenter - onBlade) * 0.35f;
	}

	// 材質ごとの追加VFX（Resource/VFX/HitMat*.vfx.json）
	const char* MaterialVfxName(Enemy::HitMaterial material) {
		switch (material) {
		case Enemy::HitMaterial::Bone:  return "HitMatBone";
		case Enemy::HitMaterial::Armor: return "HitMatArmor";
		case Enemy::HitMaterial::Wood:  return "HitMatWood";
		case Enemy::HitMaterial::Flesh:
		default:                        return "HitMatFlesh";
		}
	}

	// 材質ごとの手応えの音。火花・破片の種類と同じ分け方にしてある
	const char* MaterialHitSound(Enemy::HitMaterial material) {
		switch (material) {
		case Enemy::HitMaterial::Bone:  return GameSound::kHitBone;
		case Enemy::HitMaterial::Armor: return GameSound::kHitArmor;
		case Enemy::HitMaterial::Wood:  return GameSound::kHitWood;
		case Enemy::HitMaterial::Flesh:
		default:                        return GameSound::kHitFlesh;
		}
	}
}

PlayerWeapon::PlayerWeapon(std::string objectName) : Object3d(objectName) {}

void PlayerWeapon::Initialize() {
	Object3d::Initialize();

	GetRenderer("PlayerWeapon")->GetWorldTransform()->GetScale() = {0.5f, 1.0f, 0.5f};

	GetCollider("WeaponCollider")->category_ = CollisionCategory::PlayerWeapon;
	static_cast<OBBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().halfExtents = baseHalfExtents_;

	defaultPosition_ = {0.0f, 0.1f, -0.5f};
	defaultRotation_ = {0.0f, 90.0f, 150.0f};

	GetWorldTransform()->GetTranslation() = defaultPosition_;
	GetWorldTransform()->GetRotation() = EulerDegree(defaultRotation_);
	// 既定の置き場所は納刀モーションの最後と同じ値。Player から渡されるまではこれを背中の姿勢にする
	backPosition_ = defaultPosition_;
	backRotation_ = defaultRotation_;
}

void PlayerWeapon::Update(float deltaTime) {
	if (!player_) return;

	auto* collider = static_cast<OBBCollider*>(GetCollider("WeaponCollider"));
	// 攻撃中だけ判定を出す。
	// 多段ヒットの区切りでは1回だけ切る。CollisionManager は「前回も触れていたか」で Enter と Stay を
	// 分けるので、一度離れたことにしないと、触れたままの敵へ次の段が入らない
	collider->GetColliderData().isActive = isAttack_ && !rehitRequested_;
	rehitRequested_ = false;
	// 攻撃ごとの当たり判定の大きさ（回転攻撃などで広げる）
	const float hitboxScale = player_->IsAttack() ? player_->GetAttackData().hitboxScale : 1.0f;
	collider->GetColliderData().halfExtents = baseHalfExtents_ * hitboxScale;

	// 剣の軌跡は PlayerAttackEffect が刃先・根本の位置から描く
	Object3d::Update(deltaTime);
}

void PlayerWeapon::Draw() {
	Object3d::Draw();
}

// ======================
// 手のボーンへ持たせる
// ======================

void PlayerWeapon::InitializeGrip(BaseRenderer* modelRenderer) {
	gripRenderer_ = modelRenderer;
	gripReady_ = false;
	gripWeight_ = 0.0f;

	GlobalVariables& global = GlobalVariables::GetInstance();
	// 保存済みの設定があれば読む（無ければ下の既定値がそのまま残る）
	global.LoadFile(kGripDirectoryName, kGripGroupName);
	global.AddItem(kGripGroupName, "GripEnabled", gripEnabled_);
	global.AddItem(kGripGroupName, "GripBoneName", gripBoneName_);
	global.AddItem(kGripGroupName, "GripOffsetPosition", gripOffsetPosition_);
	global.AddItem(kGripGroupName, "GripOffsetRotation", gripOffsetRotation_);
	global.AddItem(kGripGroupName, "GripBlendSpeed", gripBlendSpeed_);

	LoadGripParams();
}

void PlayerWeapon::LoadGripParams() {
	GlobalVariables& global = GlobalVariables::GetInstance();
	gripEnabled_ = global.GetValueRef<bool>(kGripGroupName, "GripEnabled");
	gripOffsetPosition_ = global.GetValueRef<Vector3>(kGripGroupName, "GripOffsetPosition");
	gripOffsetRotation_ = global.GetValueRef<Vector3>(kGripGroupName, "GripOffsetRotation");
	gripBlendSpeed_ = global.GetValueRef<float>(kGripGroupName, "GripBlendSpeed");

	const std::string& boneName = global.GetValueRef<std::string>(kGripGroupName, "GripBoneName");
	if (!boneName.empty() && boneName != gripBoneName_) {
		gripBoneName_ = boneName;
		grip_.SetJointName(gripBoneName_);
	}
}

bool PlayerWeapon::GetGripPose(Vector3& outPosition, Quaternion& outRotation) const {
	Matrix4x4 socket{};
	if (!grip_.GetSocketMatrix(socket)) return false;

	// 手のローカル空間に置いた握りのオフセット。
	// 剣の原点は柄より上にあるので、刃の向き(+Y)へずらすと柄が手の位置に来る
	outPosition = Transform(gripOffsetPosition_, socket);

	// **行ベクトル規約の行列からクォータニオンを取り出すときは共役を取ること。**
	// QuaternionFromMatrix は列ベクトル前提なので、そのまま使うと逆向きの回転になる
	const Quaternion socketRotation = Conjugate(QuaternionFromMatrix(socket));
	// 行列で書くと「オフセット * ソケット」。クォータニオンの積は順番が逆になる
	outRotation = socketRotation * EulerDegree(gripOffsetRotation_);
	return true;
}

void PlayerWeapon::GetHoldPoseLocal(Vector3& outPosition, Quaternion& outRotation) const {
	// GetGripPose は 剣 = 手の姿勢 ∘ 握りのオフセット で作っている。ここはその逆。
	//   剣の回転 = 手の回転 * オフセット回転  →  手の回転 = 剣の回転 * オフセット回転の逆
	//   剣の位置 = 手の位置 + 手の回転で回したオフセット位置  →  手の位置 = 剣の位置 - それ
	WorldTransform* transform = const_cast<PlayerWeapon*>(this)->GetWorldTransform();
	const Quaternion swordRotation = transform->GetRotation();
	const Vector3 swordPosition = transform->GetTranslation();

	outRotation = swordRotation * Inverse(EulerDegree(gripOffsetRotation_));
	outPosition = swordPosition - RotateVector(gripOffsetPosition_, outRotation);
}

void PlayerWeapon::SetHoldMode(HoldMode mode) {
	if (mode == holdMode_) return;
	holdMode_ = mode;
	// 寄せ直し。重みが 1 のまま行き先だけ変えると、次のフレームで新しい姿勢へ瞬間移動する
	gripWeight_ = 0.0f;
}

void PlayerWeapon::UpdateGrip(float deltaTime) {
	// レンダラーの生成順によってはスキンインスタンスがまだ無いことがあるので、
	// 取れるようになってから一度だけ繋ぐ
	if (!gripReady_ && gripRenderer_ && gripRenderer_->GetSkinnedInstance()) {
		grip_.Initialize(gripRenderer_, gripBoneName_);
		gripReady_ = true;
	}
	if (gripReady_) {
		LoadGripParams();
	}

	// 振り始めたら即座に譲る。
	// ここを緩やかに抜くと、振っている最中も剣が手の方へ引っぱられて軌道が鈍る。
	// 振りへの繋ぎは PlayerStateAttack::UpdateStartup が
	// 「今の姿勢 → 制御点の1つ目」へ補間してくれるので、こちらは切るだけでよい
	// （背中から振り始めれば、それがそのまま剣を抜く動きになる）
	if (holdMode_ == HoldMode::Swing) {
		gripWeight_ = 0.0f;
		return;
	}

	Vector3 targetPosition{};
	Quaternion targetRotation{};
	if (holdMode_ == HoldMode::Back) {
		// 背中はボーンに付けず、プレイヤーのローカル空間に固定する。
		// 胴のボーンに付けると走りの上下・ひねりで剣が揺れる（それが嫌で背負わせている）
		targetPosition = backPosition_;
		targetRotation = EulerDegree(backRotation_);
	} else {
		// 手に持たせないなら、振り終えた位置に置いたまま（ボーン追従を入れる前の挙動）
		if (!gripReady_ || !gripEnabled_ || !grip_.IsValid()) {
			gripWeight_ = 0.0f;
			return;
		}
		if (!GetGripPose(targetPosition, targetRotation)) return;
	}

	// 寄せるときは時間をかける（振り終わりに剣が手へ収まっていく／背中へ回っていく動きになる）。
	// 納刀モーションを振り終えた直後は、剣がすでに背中の姿勢にあるので何も動かない
	const float rate = std::clamp(gripBlendSpeed_ * deltaTime, 0.0f, 1.0f);
	gripWeight_ += (1.0f - gripWeight_) * rate;

	WorldTransform* transform = GetWorldTransform();
	transform->GetTranslation() = Lerp(transform->GetTranslation(), targetPosition, gripWeight_);
	transform->GetRotation() = Slerp(transform->GetRotation(), targetRotation, gripWeight_);
}

#ifdef _DEBUG
void PlayerWeapon::DrawGripEditor() {
	GlobalVariables& global = GlobalVariables::GetInstance();

	static const char* const kHoldModeNames[] = { "振り（攻撃の制御点）", "手に持つ", "背中に背負う" };
	ImGui::Text("持ち方: %s （寄せ %.2f）", kHoldModeNames[static_cast<int>(holdMode_)], gripWeight_);
	ImGui::TextDisabled("背中の位置は攻撃エディタの Sheathe の最後の制御点 (%.2f, %.2f, %.2f)",
		backPosition_.x, backPosition_.y, backPosition_.z);

	if (!gripReady_) {
		ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.40f, 1.0f), "スキンモデルが見つかりません（剣は従来通り制御点で動きます）");
		return;
	}
	if (!grip_.IsValid()) {
		ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.40f, 1.0f), "ジョイント \"%s\" がモデルにありません", gripBoneName_.c_str());
	}

	ImGui::Checkbox("コンボの合間は手に持たせる", &global.GetValueRef<bool>(kGripGroupName, "GripEnabled"));
	ImGui::SetItemTooltip("切ると、コンボの合間は振り終えた位置に剣を置いたままにする（背負うのは変わらない）");

	ImGui::DragFloat3("握りの位置", &global.GetValueRef<Vector3>(kGripGroupName, "GripOffsetPosition").x, 0.01f);
	ImGui::SetItemTooltip("手のボーンから見た剣の原点。+Y が刃の向きなので、大きくすると柄を深く握る");
	ImGui::DragFloat3("握りの回転(度)", &global.GetValueRef<Vector3>(kGripGroupName, "GripOffsetRotation").x, 1.0f);
	ImGui::DragFloat("持ち替えの速さ", &global.GetValueRef<float>(kGripGroupName, "GripBlendSpeed"), 0.1f, 1.0f, 60.0f);
	ImGui::SetItemTooltip("振り終わりに剣が手へ戻る速さ（回避などで納刀モーションが出なかったときに背中へ回る速さも兼ねる）。小さいほどゆっくり");

	if (ImGui::Button("Save##Grip")) {
		global.SaveFile(kGripDirectoryName, kGripGroupName);
	}
}
#endif // _DEBUG

Vector3 PlayerWeapon::GetBladeTipWorld() {
	return Transform(bladeTipOffset_, GetWorldTransform()->GetMatWorld());
}

Vector3 PlayerWeapon::GetBladeBaseWorld() {
	return Transform(bladeBaseOffset_, GetWorldTransform()->GetMatWorld());
}

void PlayerWeapon::OnCollisionEnter(BaseCollider* other) {
	if (other->category_ == CollisionCategory::Enemy) {
		if (!player_->IsAttack()) return;

		// 攻撃の状況（空中・強攻撃・敵の強さ）を集めてスタイルスコアへ渡す
		const AttackData& attack = player_->GetAttackData();
		AttackHitContext hitCtx;
		hitCtx.attackName = player_->GetCombat()->GetCurrentAttackName();
		if (hitCtx.attackName.empty()) hitCtx.attackName = attack.name;
		hitCtx.isAir = (attack.posture == AttackPosture::Air);
		// 打ち上げ・吹き飛ばしは強攻撃として高めに評価する
		hitCtx.isStrong = (attack.knockback.type == ReactionType::Launch || attack.knockback.type == ReactionType::Knockback);
		auto* enemy = dynamic_cast<Enemy*>(other->owner_);
		if (enemy) {
			hitCtx.enemyMultiplier = enemy->GetStyleMultiplier();
		}
		scoreManager_->OnAttackHit(hitCtx);

		// ── ヒット演出（VFX・カメラシェイク・ポストエフェクト・ヒットストップ・ライト）──
		// 中身は HitEffectSystem がまとめて面倒を見る。ここでは「どこで・どちらへ・どの種類で」を決める
		const Vector3 attackerPosition = player_->GetWorldTransform()->GetTranslation();
		const Vector3 bladeTip = GetBladeTipWorld();
		const Vector3 bladeBase = GetBladeBaseWorld();
		const PlayerAttackEffect* attackEffect = player_->GetAttackEffect();

		HitEffectRequest fx;
		// 斬った場所: 刃の上で敵の体に一番近い点
		if (enemy) {
			fx.position = CalcBladeHitPosition(bladeBase, bladeTip, enemy->GetBodyCenter());
		} else if (other->owner_) {
			fx.position = other->owner_->GetWorldTransform()->GetTranslation();
		} else {
			fx.position = bladeTip;
		}

		// 火花は剣を振っている向きへ流す（斬った向きが見えるように）。
		// 刃がほとんど動いていないとき（突き・溜めの出始め）は、攻撃者から離れる向き
		const Vector3 away = SafeNormalize(fx.position - attackerPosition, Vector3{ 0.0f, 1.0f, 0.0f });
		const Vector3 tipVelocity = attackEffect ? attackEffect->GetTipVelocity() : Vector3{};
		if (Length(tipVelocity) > 1.0f) {
			fx.direction = SafeNormalize(SafeNormalize(tipVelocity, away) * 0.75f + away * 0.25f, away);
		} else {
			fx.direction = away;
		}

		fx.strength = attack.hitStopStrength;
		fx.hitStopTime = attack.hitStopTime;
		fx.hitStopIntensity = attack.hitStopIntensity;
		// スーパーアーマーで弾かれた場合は控えめな演出にする
		fx.isArmorHit = (enemy && enemy->IsKnockbackImmune());

		// 弾かれたヒットでは火花もリングも出さない。
		// 「攻撃が通っていない」ことは敵側の紫の火花（BossArmorHitSpark）が伝える
		if (!fx.isArmorHit) {
			const AttackVfxStyle style = attackEffect ? attackEffect->GetCurrentStyle() : AttackVfxStyle::Slash;
			const bool isHeavy = (style == AttackVfxStyle::Heavy || style == AttackVfxStyle::Slam || style == AttackVfxStyle::Thrust);
			// 通常斬りは白＋青の十字の光、強攻撃は白＋金の大きな光
			fx.vfxName = isHeavy ? "PlayerHitHeavy" : "PlayerHitSlash";
			fx.materialVfxName = enemy ? MaterialVfxName(enemy->GetHitMaterial()) : "";
		}
		// 大きな敵ほど火花を少し大きくする（ボスは配置スケール2で約1.4倍）
		if (enemy) {
			const float enemyScale = enemy->GetWorldTransform()->GetWorldScale().y;
			fx.sizeScale = std::clamp(std::sqrt((std::max)(enemyScale, 0.0f)), 1.0f, 1.6f);
		}
		HitEffectSystem::GetInstance().Play(fx);

		// 手応えの音。3段構えで鳴らし分ける:
		//   1. スーパーアーマーで弾かれた → 通っていないことが分かる詰まった音だけ
		//   2. 強攻撃 → 重い衝撃を材質音に重ねる（材質だけだと大振りの手応えが出ない）
		//   3. それ以外 → 材質の音
		SoundManager& sound = SoundManager::GetInstance();
		if (fx.isArmorHit) {
			sound.PlaySE(GameSound::kHitBlocked, 0.7f);
		} else {
			const AttackVfxStyle style = attackEffect ? attackEffect->GetCurrentStyle() : AttackVfxStyle::Slash;
			const bool isHeavyHit = (style == AttackVfxStyle::Heavy || style == AttackVfxStyle::Slam);
			sound.PlaySE(enemy ? MaterialHitSound(enemy->GetHitMaterial()) : GameSound::kSwordHit, 0.85f);
			if (isHeavyHit) {
				sound.PlaySE(GameSound::kHitHeavy, 0.8f);
			}
		}

		// チュートリアル対象の攻撃であれば進行させる
		TutorialState tutorialState = ResolveTutorialState(player_->GetCombat()->GetCurrentAttackName());
		if (tutorialState != TutorialState::Count) {
			player_->GetTutorialService()->StepTutorial(tutorialState);
		}
	}
}

void PlayerWeapon::OnCollisionStay(BaseCollider* other) {
	other;
}

void PlayerWeapon::OnCollisionExit(BaseCollider* other) {
	other;
}

#ifdef _DEBUG
void PlayerWeapon::DebugGui() {
	Object3d::DebugGui();

}

#endif // _DEBUG

