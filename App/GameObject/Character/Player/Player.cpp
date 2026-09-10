#include "Player.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateMove.h"
#include "World3D/Object/Renderer/RendererManager.h"
#include "World3D/Object/Renderer/PrimitiveRenderer.h"
#include "World3D/Object/Model/ModelManager.h"
#include "World3D/Collider/AABBCollider.h"
#include "World3D/Collider/OBBCollider.h"
#include "World3D/Collider/CollisionManager.h"
#include "Utility/DeltaTime.h"
#include "Utility/TimeManager.h"
#include "State/PlayerStateJump.h"
#include "State/PlayerStateAir.h"
#include "World3D/Primitive/PrimitiveLineDrawer.h"
#include "State/Attack/PlayerStateAttack.h"
#ifdef _DEBUG
#endif
#include "Scene/Transition/TransitionManager.h"
#include "State/PlayerStateDeath.h"
#include "State/PlayerStateClear.h"
#include "State/PlayerStateKnockBack.h"
#include "State/PlayerStateDodge.h"
#include "State/PlayerStateDash.h"
#include "State/PlayerStateJustDodge.h"
#include "Controller/PlayerInput.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "World3D/Object/Model/Animation/AnimationPlayer.h"
#include "World3D/Object/Model/Animation/SkinnedInstance.h"
#include "World3D/Camera/CameraManager.h"
#include "GameObject/Camera/GameCamera.h"
#include "GameObject/Character/Enemy/Component/EnemyHitbox.h"
#include "Audio/SoundManager.h"

#include <numbers>
#include <algorithm>

#include "Input/Input.h"

Player::Player(std::string objectName) : Object3d(objectName) {
	Object3d::Initialize();

	// ModelRenderer は FindModel するだけで読み込みはしないので、使うモデルはここで読んでおく。
	// TitleScene の先読みに頼っていると、そちらを整理したときに静かに壊れる
	// Alien.gltf はリグ付き（45ジョイント・15クリップ）なのでスキンモデルとして読む。
	// ModelRenderer は SkinnedModel を渡されると自動で SkinnedInstance を作るので、
	// 生成のしかたは静的モデルのときと変わらない
	ModelManager::GetInstance().LoadSkinnedModel(kModelName);
	ModelManager::GetInstance().LoadModel("Sword");

	// レンダラーの生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(kRendererName, kModelName));

	AddRenderer(RendererManager::GetInstance().FindRender(kRendererName));
	// Alien.obj は素の高さが約2.9m。オブジェクト原点はコライダー(半径0.5)の中心なので、
	// 縮めたうえで足元がコライダーの底に来るように下げる
	GetRenderer(kRendererName)->GetWorldTransform()->GetScale() = { kModelScale, kModelScale, kModelScale };
	GetRenderer(kRendererName)->GetWorldTransform()->GetTranslation().y += kModelOffsetY;
	// このモデルは正面が +Z。プレイヤーの前方向もローカル +Z（Rotate/LockOn の LookRotation と
	// 攻撃モーションが +Z へ振り抜くのがその根拠）なので、敵と違って向き補正は要らない

	// StateMachine生成
	stateMachine_ = std::make_unique<PlayerStateMachine>();
	// ステートをセットしてcurrentに登録
	stateMachine_->SetFirstState("Idle", std::make_unique<PlayerStateIdle>());
	// 各ステートを登録
	stateMachine_->AddState("Move", std::make_unique<PlayerStateMove>());
	stateMachine_->AddState("Jump", std::make_unique<PlayerStateJump>());
	stateMachine_->AddState("Air", std::make_unique<PlayerStateAir>());
	stateMachine_->AddState("Death", std::make_unique<PlayerStateDeath>());
	stateMachine_->AddState("Clear", std::make_unique<PlayerStateClear>());
	stateMachine_->AddState("Knockback", std::make_unique<PlayerStateKnockBack>());
	// 回避 → ダッシュ は1つの連続した移動アクション。ジャスト回避はその途中に割り込む
	stateMachine_->AddState("Dodge", std::make_unique<PlayerStateDodge>());
	stateMachine_->AddState("Dash", std::make_unique<PlayerStateDash>());
	stateMachine_->AddState("JustDodge", std::make_unique<PlayerStateJustDodge>());
}

void Player::Initialize() {
	combat_ = std::make_unique<PlayerCombat>();
	combat_->Initialize(this);

	GetCollider(name_)->category_ = CollisionCategory::Player;
	// レベルデータのコライダーサイズをそのまま使う
	// （旧: レベル側の×2補正を打ち消すための *0.5f があったが、×2補正の撤廃に伴い削除。実効サイズは変わらない）
	// 武器のレンダラー生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>("PlayerWeapon", "Sword"));
	// 武器用のコライダー生成
	CollisionManager::GetInstance().AddCollider(std::make_unique<OBBCollider>("WeaponCollider"));
	// 武器を生成
	weapon_ = std::make_unique<PlayerWeapon>("PlayerWeapon");
	// 武器にレンダラーを追加
	weapon_->AddRenderer(RendererManager::GetInstance().FindRender("PlayerWeapon"));
	// 武器にコライダーを追加
	weapon_->AddCollider(CollisionManager::GetInstance().FindCollider("WeaponCollider"));
	// 武器の初期化
	weapon_->Initialize();
	// 武器のワールドトランスフォームをプレイヤーの子にする
	weapon_->GetWorldTransform()->SetParent(GetWorldTransform());
	// プレイヤークラスのポインタを武器クラスに渡す
	weapon_->SetPlayer(this);

	scoreManager = std::make_unique<StylishScoreManager>();
	scoreManager->Initialize();

	weapon_->SetScoreManager(scoreManager.get());

	// HP表示用のハートスプライトを生成
	hearts_.resize(maxHp_);
	for (int32_t i = 0; i < maxHp_; ++i) {
		// UI要素の初期化
		hearts_[i] = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "heart" + std::to_string(i), "Heart.png");
		hearts_[i]->SetSize({64.0f, 64.0f});
		hearts_[i]->SetAnchorPoint({0.5f, 0.5f});
		hearts_[i]->SetPosition({50.0f + i * 50.0f, 50.0f}); // 左上に横並びで配置
	}

	hitStop_ = std::make_unique<HitStop>();

	// 攻撃ヒット時のポストエフェクト。被弾ビネットより前にチェーンさせたいので先に登録する
	hitPostEffect_ = std::make_unique<HitPostEffect>();
	hitPostEffect_->Initialize();

	hitVignette_ = std::make_unique<HitVignetteEffect>();
	hitVignette_->Initialize();

	// プレイヤーに追従するポイントライト（攻撃ヒット時にフラッシュする）
	characterLight_ = std::make_unique<CharacterLight>();
	characterLight_->Initialize("PlayerLight", Vector4{ 0.45f, 0.65f, 1.0f, 1.0f });

	// 被弾時に体と武器を一瞬光らせる。体だけだと剣が暗いまま浮くので武器も対象に入れる
	hitFlash_ = std::make_unique<HitFlashComponent>();
	hitFlash_->AddRenderer(GetRenderer(kRendererName));
	hitFlash_->AddRenderer(weapon_->GetRenderer("PlayerWeapon"));

	// ── 死亡演出 ──
	// 画面効果（グレースケール＋暗転ビネット）。ポストエフェクトはシーンをまたいで
	// 生き続けるので、Initialize() の中で必ず切った状態へ戻している
	deathScreen_ = std::make_unique<DeathScreenEffect>();
	deathScreen_->Initialize();

	// 最後に体と武器を溶かして消す。縁の色はプレイヤーのライトに合わせた青白
	deathDissolve_ = std::make_unique<DissolveOutEffect>();
	deathDissolve_->AddRenderer(GetRenderer(kRendererName));
	deathDissolve_->AddRenderer(weapon_->GetRenderer("PlayerWeapon"));
	deathDissolve_->SetEdgeColor({ 0.45f, 0.75f, 1.0f, 6.0f });
	// レンダラーが作り直されていない経路（エディタからのシーン再読込など）でも
	// 溶けたままにならないよう、ここで上書きを解除しておく
	deathDissolve_->Reset();

	hudAlpha_ = 1.0f;

	// ── 回避・ダッシュ ──
	dodgeParams_.RegisterAndLoad();
	dodgeRuntime_ = PlayerDodgeRuntime{};
	dodgeInvincibleTimer_ = 0.0f;
	dodgeCooldownTimer_ = 0.0f;
	justDodgeSlowTimer_ = 0.0f;

	// 回避・ダッシュ中の残像。武器の軌跡と同じリボンの仕組みを、体の中心に沿って引いている
	dodgeTrail_ = std::make_unique<WeaponTrail>();
	dodgeTrail_->Initialize();
	dodgeTrail_->SetLifetime(dodgeParams_.trailLifetime);
	dodgeTrailActive_ = false;

	justDodgeEffect_ = std::make_unique<JustDodgeEffect>();
	justDodgeEffect_->Initialize();
}

// ステートと戦闘状態から再生するクリップを決めて流す。
// 各ステートの Enter に Play を撒くと「攻撃が終わったら元のクリップに戻す」が漏れやすいので、
// 毎フレームここで決め直す方式にしている。
// AnimationPlayer::Play は同じクリップなら何もしないので、毎フレーム呼んで問題ない。
//
// クリップを変えたいときはこの対応表をいじること。Alien.gltf が持つのは以下の15種:
//   Idle / IdleHold / Standing / Sitting / Walk / Run / RunHold / Jump / RunningJump /
//   Roll / Punch / SwordSlash / Death / Swimming / Clapping
void Player::UpdateAnimation() {
	AnimationPlayer* anim = GetAnimationPlayer();
	if (!anim) return;

	// ── 攻撃中は斬りモーションを最優先 ──
	// コンボで攻撃が切り替わったら頭から出し直す（同じ技を連打しても振り直したいので名前で見る）
	if (combat_ && combat_->IsAttacking()) {
		const std::string& attackName = combat_->GetCurrentAttackName();
		const bool isNewSwing = (attackName != lastAttackName_);
		lastAttackName_ = attackName;

		anim->Play(kClipAttack, false, 0.05f, isNewSwing);

		// 体の「振り切る瞬間」が武器の振り抜きと重なるように再生速度を決める。
		// 武器は preDelay で構えに移動し、attackDuration の間に CatmullRom で振り抜くので、
		// 斬る瞬間は preDelay + attackDuration/2 あたり。
		// GetDuration() は再生中クリップの長さなので、必ず Play の後に取ること
		// （先に取ると切り替え前＝待機モーション4.17秒の長さで割ることになり、初回だけ数倍速で飛ぶ）
		const AttackData data = GetAttackData();
		const float weaponImpact = data.preDelay + data.attackDuration * 0.5f;
		const float clipImpact = anim->GetDuration() * kAttackClipImpactRatio;
		const float speed = (weaponImpact > 0.01f && clipImpact > 0.01f) ? (clipImpact / weaponImpact) : 1.0f;
		anim->SetSpeed(std::clamp(speed, kAttackSpeedMin, kAttackSpeedMax));
		return;
	}
	lastAttackName_.clear();
	anim->SetSpeed(1.0f);

	// ── 通常時はステート名で決める ──
	const PlayerStateBase* state = stateMachine_ ? stateMachine_->GetCurrentState() : nullptr;
	const std::string name = state ? state->GetDebugName() : "Idle";

	if (name == "Death") {
		anim->Play(kClipDeath, false, 0.15f);
	} else if (name == "Clear") {
		anim->Play(kClipClear, true, 0.25f);
	} else if (name == "Knockback") {
		anim->Play(kClipKnockBack, false, 0.05f);
	} else if (name == "Dodge" || name == "JustDodge") {
		// ノックバックと同じ前転クリップなので、回避で入り直したときは頭から出し直す
		anim->Play(kClipDodge, false, 0.05f, dodgeAnimRestart_);
		dodgeAnimRestart_ = false;
		// 前転クリップは回避時間(0.25秒)より長いので、収まるように速める
		const float duration = anim->GetDuration();
		const float speed = (duration > 0.01f && dodgeParams_.dodgeDuration > 0.01f)
			? duration / dodgeParams_.dodgeDuration : 1.0f;
		anim->SetSpeed(std::clamp(speed, kDodgeSpeedMin, kDodgeSpeedMax));
	} else if (name == "Dash") {
		anim->Play(kClipDash, true, 0.12f);
		anim->SetSpeed(kDashClipSpeed);
	} else if (name == "Jump" || name == "Air") {
		anim->Play(kClipJump, false, 0.1f);
	} else if (name == "Move") {
		anim->Play(kClipMove, true, 0.15f);
	} else {
		anim->Play(kClipIdle, true, 0.2f);
	}
}

AnimationPlayer* Player::GetAnimationPlayer() {
	BaseRenderer* renderer = GetRenderer(kRendererName);
	if (!renderer) return nullptr;
	// 静的モデルに戻した場合はスキンインスタンスが無いので、その場合は素通りさせる
	SkinnedInstance* instance = renderer->GetSkinnedInstance();
	return instance ? instance->GetPlayer() : nullptr;
}

bool Player::IsDying() const {
	const PlayerStateBase* current = stateMachine_ ? stateMachine_->GetCurrentState() : nullptr;
	return current && std::string(current->GetDebugName()) == "Death";
}

void Player::Update(float deltaTime) {
	// 入力とロックオンはシーン側が接続する。エディタで生成した直後など未接続の間は
	// トランスフォームの更新だけして動かさない（次のシーン読み込みで有効になる）
	if (!input_ || !lockOn_) {
		Object3d::Update(deltaTime);
		return;
	}

	// 死亡演出中はシーン側が世界の時間を落として敵を止める（GameSceneStatePlay）。
	// プレイヤー自身はその影響を受けずに倒れ切りたいので、ここだけ実時間で進める
	if (IsDying()) {
		deltaTime = DeltaTime::GetDeltaTime();
	}

	// 回避・ダッシュの調整値はエディタから触れるよう毎フレーム読み直す
	dodgeParams_.Apply();

	// ジャスト回避のスローモーションは実時間で数える。
	// スケール済みの時間で数えると、遅くした分だけ演出が伸びて自分の首を絞める
	const float realDelta = DeltaTime::GetDeltaTime();
	if (justDodgeSlowTimer_ > 0.0f) {
		justDodgeSlowTimer_ -= realDelta;
	}
	justDodgeEffect_->Update(realDelta);

	hitStop_->Update(deltaTime);
	float dt = deltaTime * hitStop_->GetTimeScale();
	// パーティクルなど自分でヒットストップを持たない系統にも時間停止を伝える
	TimeManager::RequestGameTimeScale(hitStop_->GetTimeScale());
	// ジャスト回避のスローもパーティクル・アニメーションへ伝える（最も遅い要求が採用される）
	TimeManager::RequestGameTimeScale(GetWorldTimeScale());

	// Rキーを押したら死亡演出が流れる ← デバッグ用
	if (Input::GetInstance().TriggerKey(DIK_R)) {
		ChangeState("Death");
	}

	scoreManager->Update();

	LockOn();

	// 無敵時間のカウントダウン
	if (invincibleTimer_ > 0.0f) {
		invincibleTimer_ -= dt;
	}
	// 回避の無敵とクールダウン
	if (dodgeInvincibleTimer_ > 0.0f) {
		dodgeInvincibleTimer_ -= dt;
	}
	if (dodgeCooldownTimer_ > 0.0f) {
		dodgeCooldownTimer_ -= dt;
	}

	// 被弾ビネットの更新
	hitVignette_->Update(dt);

	// ヒット時のポストエフェクトはヒットストップ中も実時間で減衰させる
	hitPostEffect_->Update(deltaTime);

	weapon_->Update(dt);

	// 回避・ダッシュの残像。点を積むのは移動中だけだが、
	// 積んだ点が消えるまでは止まった後も更新し続ける必要がある。
	// 発生条件をステートから引くことで、被弾や死亡で回避が中断されても消し忘れない
	dodgeTrailActive_ = IsDodging() || IsDashing();
	if (dodgeTrailActive_) {
		const Vector3 pos = GetWorldTransform()->GetTranslation();
		dodgeTrail_->AddPoint(pos + Vector3{ 0.0f, kTrailTopOffsetY, 0.0f },
			pos + Vector3{ 0.0f, kTrailBottomOffsetY, 0.0f });
	}
	dodgeTrail_->Update(dt);

	// ノックバック中は戦闘状態に関わらず常にステートを更新する
	bool isKnockbackState = (stateMachine_->GetCurrentState() && std::string(stateMachine_->GetCurrentState()->GetDebugName()) == "Knockback");
	if (isKnockbackState) {
		stateMachine_->UpdateCurrentState(*this, dt);
	} else if (!combat_->IsAttacking()) {
		stateMachine_->UpdateCurrentState(*this, dt);
	}
	combat_->Update(dt);
	// 入力に応じたコマンドを実行
	for (auto& cmd : input_->GetCommands()) {
		ExecuteCommand(cmd);
	}

	// 確定したステート・戦闘状態でクリップを決める。
	// ポーズの更新は Object3d::Update の中（レンダラー更新）で走るので、その手前で呼ぶ
	UpdateAnimation();

	// 移動処理
	GetWorldTransform()->GetTranslation() += velocity_ * dt;
	velocity_ += acceleration_ * dt;

	Object3d::Update(dt);

	// 強制戦闘エリアなどで移動範囲が設定されている場合は水平方向を範囲内にクランプする
	// （範囲は床の向きに合わせて回転していることがあるので、軸に沿って押し戻す）
	if (hasMovementBounds_) {
		Vector3& pos = GetWorldTransform()->GetTranslation();
		Vector3 inwardNormal{};
		movementBounds_.ClampPosition(pos, inwardNormal);
	}

	// キャラクター追従ライトの更新（ヒットストップ中はフラッシュの減衰も止まる）
	characterLight_->Update(GetWorldTransform()->GetTranslation(), dt);

	// 被弾フラッシュ。dt（ヒットストップ適用後）で進めるので、時間が止まっている間は白いまま保持される
	hitFlash_->Update(dt);

	// 接地フラグを毎フレーム切っておく
	onGround_ = false;

	// HPのハート。死亡演出中は hudAlpha_ が下がってフェードアウトする
	for (size_t i = 0; i < hearts_.size(); ++i) {
		if (i < hp_) {
			hearts_[i]->SetColor({1.0f, 1.0f, 1.0f, hudAlpha_}); // HP分は表示
		} else {
			hearts_[i]->SetColor({0.0f, 0.0f, 0.0f, 0.3f * hudAlpha_}); // 残りは半透明で表示
		}
		hearts_[i]->Update();
	}
}

void Player::Draw() {
	weapon_->Draw();
	Object3d::Draw();

}

void Player::DrawEffect() {
	combat_->Draw();
	weapon_->DrawEffect();
	dodgeTrail_->Draw();
}

void Player::DrawUI() {

}


void Player::ChangeState(const std::string& stateName) {
	stateMachine_->ChangeState(*this, stateName);
}

void Player::ExecuteCommand(const PlayerCommand& command) {
	// デス状態では全コマンドを無視する
	auto* cur = stateMachine_->GetCurrentState();
	if (cur && std::string(cur->GetDebugName()) == "Death") return;

	// 回避だけは攻撃中でもここで直接処理する。
	// 各ステートの ExecuteCommand へ配ると、攻撃中はステートに届かない（下の分岐で止まる）ため、
	// 「攻撃を回避でキャンセルする」というDMC系の基本動作が成立しない
	if (command.action == PlayerAction::Dodge) {
		if (CanStartDodge()) {
			combat_->InterruptCombat();
			ChangeState("Dodge");
		}
		return;
	}

	// 回避・ダッシュ中に攻撃へ移る場合は、先に移動ステートを畳んでおく。
	// 攻撃中はステートの更新が止まるので、放置すると攻撃後に古いタイマーのまま再開してしまう。
	// 水平速度もここで落とす（移動しない攻撃だと、回避の勢いのまま滑りながら斬ってしまう）
	if (command.action == PlayerAction::Attack && (IsDodging() || IsDashing())) {
		velocity_.x = 0.0f;
		velocity_.z = 0.0f;
		ChangeState("Idle");
	}

	if (!combat_->IsAttacking()) {
		stateMachine_->ExecuteCommand(*this, command);
	}
	combat_->ExecuteCommand(command);
}

bool Player::IsDodging() const {
	const PlayerStateBase* current = stateMachine_ ? stateMachine_->GetCurrentState() : nullptr;
	if (!current) return false;
	const std::string name = current->GetDebugName();
	return name == "Dodge" || name == "JustDodge";
}

bool Player::IsDashing() const {
	const PlayerStateBase* current = stateMachine_ ? stateMachine_->GetCurrentState() : nullptr;
	return current && std::string(current->GetDebugName()) == "Dash";
}

float Player::GetWorldTimeScale() const {
	return (justDodgeSlowTimer_ > 0.0f) ? dodgeParams_.justDodgeTimeScale : 1.0f;
}

bool Player::CanStartDodge() const {
	if (dodgeCooldownTimer_ > 0.0f) return false;
	// 空中では回避できない。宙に浮いたまま滑る絵にならないよう接地を必須にしている
	if (!onGround_) return false;

	const PlayerStateBase* current = stateMachine_ ? stateMachine_->GetCurrentState() : nullptr;
	if (!current) return false;

	// 回避中の再入力は無視（ダッシュ中は方向を変える手段として許可する）
	const std::string name = current->GetDebugName();
	if (name == "Death" || name == "Clear" || name == "Knockback") return false;
	if (name == "Dodge" || name == "JustDodge") return false;

	return true;
}

Vector3 Player::CalcDodgeDirection() {
	// スティックが入っていれば画面基準の入力方向（仕様書 §5.2 の推奨）
	const Vector3 inputDir = GetMoveDirection();
	if (Length(inputDir) > 0.01f) return inputDir;

	// 入力が無ければキャラクターの前方向。このモデルの正面はローカル +Z
	Vector3 forward = TransformNormal(Vector3{ 0.0f, 0.0f, 1.0f }, GetWorldTransform()->GetMatWorld());
	forward.y = 0.0f;
	const float length = Length(forward);
	return (length > 0.0001f) ? forward * (1.0f / length) : Vector3{ 0.0f, 0.0f, 1.0f };
}

void Player::FaceDirection(const Vector3& direction) {
	// ロックオン中は敵の方を向いたまま横へ滑ってほしいので向きは変えない
	if (lockOn_ && lockOn_->IsLockOn()) return;
	if (Length(direction) < 0.001f) return;

	// Rotate() と同じく、LookRotation へ渡す前にXを反転させる
	Vector3 dir = direction;
	dir.x *= -1.0f;
	GetWorldTransform()->GetRotation() = LookRotation(dir);
}

void Player::OnDodgeStart() {
	const PlayerDodgeParams& params = dodgeParams_;

	dodgeInvincibleTimer_ = params.invincibleTime;
	dodgeCooldownTimer_ = params.dodgeDuration + params.cooldown;
	dodgeAnimRestart_ = true;

	// 残像は短く控えめに。ダッシュに入ったところで濃く長くする。
	// トレイルは加算合成なので、明るい床の上でも色が乗るように青を濃いめにしている
	dodgeTrail_->Clear();
	dodgeTrail_->SetLifetime(params.trailLifetime);
	dodgeTrail_->SetTintColor({ 0.30f, 0.65f, 1.0f, 0.85f });

	// 足元の砂埃。回避方向の逆へ舞わせる
	const Vector3 feet = GetWorldTransform()->GetTranslation() + Vector3{ 0.0f, kModelOffsetY, 0.0f };
	ParticleManager::GetInstance().PlayVFX("DodgeDust", feet, dodgeRuntime_.direction * -1.0f);

	SoundManager::GetInstance().PlaySE("Dodge", 0.6f);

	// 追従ライトのフラッシュはここでは焚かない。
	// 回避は頻繁に使うので、毎回床が真っ白に照らされると画面がうるさくなる
}

void Player::OnDashStart() {
	const PlayerDodgeParams& params = dodgeParams_;

	// 残像を強くする。回避との差をここではっきり付ける（仕様書 §10.3）
	dodgeTrail_->SetLifetime(params.trailLifetime * 1.6f);
	dodgeTrail_->SetTintColor({ 0.55f, 0.85f, 1.0f, 1.0f });

	SoundManager::GetInstance().PlaySE("Dash", 0.7f);

	// ダッシュ開始の瞬間だけ画角を広げる。
	// 速度に応じた常時のFOV変化はカメラ側が別に持っているので、ここは立ち上がりの「蹴り」だけ
	if (auto* camera = dynamic_cast<GameCamera*>(CameraManager::GetInstance().GetActiveCamera())) {
		camera->AddFovPunch(params.dashFovPunch);
	}
}

void Player::OnJustDodge() {
	const PlayerDodgeParams& params = dodgeParams_;

	dodgeRuntime_.justDodgePending = false;
	dodgeRuntime_.justDodgeUsed = true;

	// ダメージは TakeDamage 側で既に弾いている。ここは無敵の延長と演出だけ
	dodgeInvincibleTimer_ += params.justDodgeInvincibleAdd;
	justDodgeSlowTimer_ = params.justDodgeSlowTime;

	// 衝撃波は「攻撃してきた側 → プレイヤー」の向きに立てる
	const Vector3 playerPos = GetWorldTransform()->GetTranslation();
	Vector3 fromAttacker = playerPos - dodgeRuntime_.justDodgeInfo.attackerPosition;
	fromAttacker.y = 0.0f;
	const float length = Length(fromAttacker);
	const Vector3 direction = (length > 0.001f) ? fromAttacker * (1.0f / length) : Vector3{ 0.0f, 1.0f, 0.0f };

	justDodgeEffect_->Play(playerPos, direction, params.justDodgeShake);
	characterLight_->Flash();
	hitFlash_->Start();
}

Vector3 Player::GetMoveDirection() const {
	BaseCamera* camera = CameraManager::GetInstance().GetActiveCamera();
	if (!camera) return {};
	// 入力のcontext
	const PlayerInputContext& context = input_->GetContext();
	// 移動中じゃなければ処理しない
	if (!context.isMove) return {};

	// 入力方向を取得
	Vector3 inputDir = {context.move.x, 0.0f, context.move.y};
	// 大きさが小さければ処理しない
	if (Length(inputDir) < 0.01f) return {};
	// 入力方向を正規化
	inputDir = Normalize(inputDir);
	// カメラ基準で移動させる
	// カメラの前方向を取得
	Vector3 camForward = camera->GetForward();
	camForward.y = 0.0f;
	camForward = Normalize(camForward);
	// カメラの右方向を取得
	Vector3 camRight = camera->GetRight();
	camRight.y = 0.0f;
	camRight = Normalize(camRight);
	// カメラの向きと入力方向から移動方向を作成
	Vector3 moveDir = camRight * inputDir.x + camForward * inputDir.z;
	// 正規化して返す
	return Normalize(moveDir);
}

void Player::Move(Vector3 moveDir, float) {
	// y軸の速度はそのままにしておく
	float velocityY = velocity_.y;

	velocity_ = moveDir * moveSpeed_;
	velocity_.y = velocityY;
}

void Player::Rotate(Vector3 moveDir, float deltaTime) {
	// ロックオンしているなら回転させない
	if (lockOn_->IsLockOn()) return;
	// 移動方向が無ければ処理しない
	if (Length(moveDir) < 0.001f) return;

	moveDir.x *= -1.0f;
	// 移動方向から目標の回転を作成
	Quaternion targetRot = LookRotation(moveDir);
	// 現在の回転を取得
	Quaternion& currentRot = GetWorldTransform()->GetRotation();
	// 補完して回転を更新
	currentRot = Slerp(currentRot, targetRot, rotateSpeed_ * deltaTime);
}

void Player::LockOn() {
	if (lockOn_->IsLockOn()) {
		// 現在のターゲットを取得
		auto* target = lockOn_->GetCurrentTarget();
		// ターゲットへのベクトルを計算
		Vector3 toTarget = target->GetWorldPosition() - GetWorldTransform()->GetTranslation();

		// ターゲット方向に向く
		Vector3 direction = Normalize(toTarget);
		direction.x *= -1.0f;
		Quaternion lookRot = LookRotation(direction);
		// 回転を適用
		GetWorldTransform()->GetRotation() = lookRot;
	}
}

void Player::TakeDamage(const DamageInfo& info) {
	// デス状態・無敵時間中は被ダメージなし
	auto* cur = stateMachine_->GetCurrentState();
	if (cur && std::string(cur->GetDebugName()) == "Death") return;

	// 優先順位は ジャスト回避 ＞ 通常無敵 ＞ 被弾（仕様書 §15）。
	// 回避中なら通常の無敵処理より先にジャスト回避を成立させる。
	// 実際の演出と無敵の延長は、次の更新で Dodge ステートが JustDodge へ移ってから行う
	if (IsDodging() && !dodgeRuntime_.justDodgeUsed) {
		dodgeRuntime_.justDodgePending = true;
		dodgeRuntime_.justDodgeInfo = info;
		return;
	}

	if (invincibleTimer_ > 0.0f) return;
	// 回避の無敵。ジャスト回避を使い切った後やダッシュへ移った後も、残っている間は当たらない
	if (dodgeInvincibleTimer_ > 0.0f) return;
	// トレーニングの常時無敵。のけぞりも出さず、敵の攻撃を素通りさせる
	if (invincible_) return;

	hp_ -= static_cast<int32_t>(info.damage);
	invincibleTimer_ = 1.2f;

	pendingDamageInfo_ = info;

	// 被弾ペナルティ：スタイルポイントを減らしコンボを打ち切る
	if (scoreManager) {
		scoreManager->OnDamage();
	}

	// 被弾ビネットフラッシュ
	hitVignette_->Play();
	// 敵の攻撃がヒットしたのでライトを強く光らせる
	characterLight_->Flash();
	// 体と武器も一瞬光らせる（画面端のビネットだけだと、被弾した本人が分かりにくい）
	hitFlash_->Start();

	// 攻撃を強制中断
	combat_->InterruptCombat();

	if (hp_ <= 0.0f) {
		// とどめの一撃。フィニッシュらしく重めに止めてから死亡演出へ入る
		// （吹き飛びの初速・カメラ・画面効果は PlayerStateDeath::Enter が受け持つ）
		hitStop_->Start(kDeathHitStopTime, kDeathHitStopIntensity, HitStopStrength::Heavy);
		// 被弾の赤いビネットは死亡の暗転とぶつかるので止める
		hitVignette_->Stop();
		// ジャスト回避の演出が残っていると、白フラッシュとスローが暗転に食い込む
		justDodgeSlowTimer_ = 0.0f;
		justDodgeEffect_->Stop();
		ChangeState("Death");
	} else {
		stateMachine_->ChangeState(*this, "Knockback");
	}
}

void Player::OnCollisionEnter(BaseCollider* other) {
	// 敵の武器に当たったら被ダメージ
	if (other->category_ == CollisionCategory::EnemyWeapon) {
		if (!other->owner_) return;

		// 攻撃してきた側の**ワールド**座標。GetTranslation() はローカル座標なので、
		// 敵の子になっている武器・判定では「攻撃者から離れる方向」にならない
		Vector3 attackerPos = other->owner_->GetWorldTransform()->GetWorldPos();
		Vector3 playerPos = GetWorldTransform()->GetTranslation();
		Vector3 dir = playerPos - attackerPos;
		dir.y = 0.0f;
		dir = (Length(dir) > 0.001f) ? Normalize(dir) : Vector3{0.0f, 0.0f, -1.0f};

		// ボーン追従の判定（噛みつき・叩きつけ等）は攻撃ごとにダメージが違うので、
		// 判定側が持っている値を使う。武器を振る敵（剣を持つ雑魚）は従来どおりの既定値
		DamageInfo info;
		if (auto* hitbox = dynamic_cast<EnemyHitbox*>(other->owner_)) {
			info = hitbox->GetDamageInfo();
		} else {
			info.damage = 1.0f;
			info.type = ReactionType::Knockback;
			info.impulseForce = 15.0f;
			info.upwardRatio = 0.4f;
			info.stunTime = 0.7f;
		}
		info.direction = dir;
		info.hitPosition = playerPos;
		info.attackerPosition = attackerPos;

		TakeDamage(info);
		return;
	}

	if (other->category_ == CollisionCategory::Ground) {
		ResolveGroundCollision(other);
	} else if (other->category_ == CollisionCategory::Enemy) {
		ResolveCharacterCollision(other);
	}
}

void Player::OnCollisionStay(BaseCollider* other) {
	if (other->category_ == CollisionCategory::Ground) {
		ResolveGroundCollision(other);
	} else if (other->category_ == CollisionCategory::Enemy) {
		ResolveCharacterCollision(other);
	}
}

void Player::ResolveGroundCollision(BaseCollider* other) {
	BaseCollider* playerCollider = GetCollider("Player");

	PenetrationResult result = CollisionManager::GetInstance().CalculatePenetration(playerCollider, other);
	if (!result.hit) return;

	GetWorldTransform()->GetTranslation() += result.normal * result.depth;

	if (result.normal.y > 0.5f) {
		// 上に当たってる（接地）
		GetWorldTransform()->GetTranslation().y -= 0.1f;
		//velocity_.y = 0.0f;
		onGround_ = true;
	}
}

// 敵とのめり込みは水平方向だけで解消する。
// キャラ同士の箱は上下の重なりが最小になりやすく、最小重なり軸(MTV)をそのまま使うと
// 乗られた側が真下（＝床の中）へ押し込まれ、そのまま床を貫通して落ちてしまう
void Player::ResolveCharacterCollision(BaseCollider* other) {
	BaseCollider* playerCollider = GetCollider("Player");

	PenetrationResult result = CollisionManager::GetInstance().CalculatePenetration(
		playerCollider, other, CollisionManager::PenetrationAxis::HorizontalOnly);
	if (!result.hit) return;

	GetWorldTransform()->GetTranslation() += result.normal * result.depth;
}

void Player::OnCollisionExit(BaseCollider* other) {
	other;
}
